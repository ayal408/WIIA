import importlib.util
import json
import tempfile
import unittest
import threading
import urllib.request
from wsgiref.simple_server import make_server, WSGIRequestHandler
from pathlib import Path

spec = importlib.util.spec_from_file_location('tracker', Path(__file__).parents[1] / 'app.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class TrackerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.db = Path(self.temp.name) / 'db.sqlite'
        self.key = 'a' * 40
        self.app = mod.create_app(self.db, self.key, 'https://tracker.example')

    def tearDown(self):
        self.temp.cleanup()

    def call(self, path, method='GET', token=''):
        result = {}
        def start(status, headers):
            result.update(status=int(status.split()[0]), headers=dict(headers))
        body = b''.join(self.app({'PATH_INFO': path, 'REQUEST_METHOD': method, 'HTTP_AUTHORIZATION': 'Bearer ' + token}, start))
        result['body'] = body
        if body and result['headers']['Content-Type'].startswith('application/json'):
            result['json'] = json.loads(body)
        return result

    def create(self):
        response = self.call('/api/tracks', 'POST', self.key)
        self.assertEqual(response['status'], 201)
        return response['json']

    def test_authentication_and_separate_management_secret(self):
        self.assertEqual(self.call('/api/tracks', 'POST')['status'], 401)
        item = self.create()
        route = '/api/tracks/' + item['id']
        self.assertEqual(self.call(route, token=self.key)['status'], 404)
        self.assertEqual(self.call(route, token=item['manage_token'])['json']['image_requests'], 0)
        other = self.create()
        self.assertEqual(self.call(route, token=other['manage_token'])['status'], 404)

    def test_pixel_only_counts_after_arm_and_head_does_not_count(self):
        item = self.create()
        pixel = '/p/' + item['id'] + '.gif'
        route = '/api/tracks/' + item['id']
        token = item['manage_token']
        self.assertEqual(self.call(pixel)['body'], mod.GIF)
        self.assertEqual(self.call(route, token=token)['json']['image_requests'], 0)
        self.assertEqual(self.call(route + '/arm', 'POST', token)['status'], 200)
        self.call(pixel, 'HEAD')
        self.call(pixel)
        self.call(pixel)
        state = self.call(route, token=token)['json']
        self.assertEqual(state['image_requests'], 2)
        self.assertIsInstance(state['last_request'], int)
        self.call(route + '/disable', 'POST', token)
        self.call(pixel)
        self.assertEqual(self.call(route, token=token)['json']['image_requests'], 2)
        self.call(route, 'DELETE', token)
        self.assertEqual(self.call(route, token=token)['status'], 404)
        self.assertEqual(self.call(pixel)['body'], mod.GIF)
        self.assertIn('no-store', self.call(pixel)['headers']['Cache-Control'])

    def test_expired_tracks_cannot_count_or_arm(self):
        item = self.create()
        with mod.sqlite3.connect(self.db) as db:
            db.execute('UPDATE tracks SET expires=0, active=1')
        route = '/api/tracks/' + item['id']
        self.call('/p/' + item['id'] + '.gif')
        state = self.call(route, token=item['manage_token'])['json']
        self.assertTrue(state['expired'])
        self.assertEqual(state['image_requests'], 0)
        self.assertEqual(self.call(route + '/arm', 'POST', item['manage_token'])['status'], 410)
        self.create()
        self.assertEqual(self.call(route, token=item['manage_token'])['status'], 404)

    def test_config_and_unknown_pixel(self):
        for base in ['http://example.com', 'https://user:password@example.com', 'https://example.com/path']:
            with self.assertRaises(ValueError):
                mod.create_app(self.db, self.key, base)
        with self.assertRaises(ValueError):
            mod.create_app(self.db, 'short', 'https://example.com')
        with self.assertRaises(ValueError):
            mod.create_app(self.db, 'REPLACE_WITH_A_NEW_RANDOM_KEY_AT_LEAST_32_CHARACTERS', 'https://example.com')
        self.assertEqual(self.call('/p/' + 'x' * 32 + '.gif')['body'], mod.GIF)
        with mod.sqlite3.connect(self.db) as db:
            fields = [r[1] for r in db.execute('PRAGMA table_info(tracks)')]
        self.assertNotIn('ip', fields)
        self.assertNotIn('recipient', fields)



    def test_real_http_create_arm_and_image_request(self):
        class QuietHandler(WSGIRequestHandler):
            def log_message(self, *args):
                pass
        with make_server('127.0.0.1', 0, self.app, handler_class=QuietHandler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            origin = 'http://127.0.0.1:' + str(server.server_port)
            try:
                request = urllib.request.Request(origin + '/api/tracks', method='POST', headers={'Authorization': 'Bearer ' + self.key})
                with urllib.request.urlopen(request) as response:
                    item = json.load(response)
                route = '/api/tracks/' + item['id']
                request = urllib.request.Request(origin + route + '/arm', method='POST', headers={'Authorization': 'Bearer ' + item['manage_token']})
                urllib.request.urlopen(request).close()
                with urllib.request.urlopen(origin + '/p/' + item['id'] + '.gif') as response:
                    self.assertEqual(response.headers['Content-Type'], 'image/gif')
                    self.assertEqual(response.read(), mod.GIF)
                request = urllib.request.Request(origin + route, headers={'Authorization': 'Bearer ' + item['manage_token']})
                with urllib.request.urlopen(request) as response:
                    self.assertEqual(json.load(response)['image_requests'], 1)
            finally:
                server.shutdown()
                thread.join(timeout=2)


if __name__ == '__main__':
    unittest.main()
