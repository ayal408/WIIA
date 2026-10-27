"""Personal, opt-in email image-request tracker. No mail bodies, recipients, IPs or UA stored."""
import base64
from contextlib import contextmanager
import hashlib
import hmac
import json
import os
import re
import secrets
import sqlite3
import time
from pathlib import Path
from urllib.parse import urlsplit

GIF = base64.b64decode('R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7')
TOKEN = r'[A-Za-z0-9_-]{32}'


def create_app(db_path, api_key, public_base, ttl_days=30):
    base = urlsplit(public_base)
    if (base.scheme != 'https' and not (base.scheme == 'http' and base.hostname in ('localhost', '127.0.0.1'))) or not base.hostname or base.username or base.password or base.path not in ('', '/') or base.query or base.fragment:
        raise ValueError('PUBLIC_BASE_URL must be an HTTPS origin (localhost HTTP is development only)')
    if not isinstance(api_key, str) or len(api_key) < 32 or api_key.startswith('REPLACE_'):
        raise ValueError('TRACKER_API_KEY must contain at least 32 characters')
    if not 1 <= int(ttl_days) <= 90:
        raise ValueError('TRACKER_TTL_DAYS must be between 1 and 90')
    origin = public_base.rstrip('/')
    path = Path(db_path)
    path.parent.mkdir(parents=True, exist_ok=True)

    @contextmanager
    def connect():
        db = sqlite3.connect(path, timeout=15)
        db.row_factory = sqlite3.Row
        try:
            with db:
                yield db
        finally:
            db.close()

    with connect() as db:
        db.execute('CREATE TABLE IF NOT EXISTS tracks (id TEXT PRIMARY KEY, secret_hash TEXT NOT NULL, created INTEGER NOT NULL, expires INTEGER NOT NULL, active INTEGER NOT NULL DEFAULT 0, opens INTEGER NOT NULL DEFAULT 0, first_open INTEGER, last_open INTEGER)')
    os.chmod(path, 0o600)

    def app(env, start):
        method, route = env.get('REQUEST_METHOD', ''), env.get('PATH_INFO', '')
        now = int(time.time())
        headers = [('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0'), ('Pragma', 'no-cache'), ('X-Content-Type-Options', 'nosniff'), ('Referrer-Policy', 'no-referrer'), ('X-Frame-Options', 'DENY')]

        def respond(code, data, kind='application/json; charset=utf-8'):
            payload = json.dumps(data).encode() if isinstance(data, dict) else data
            start(code, headers + [('Content-Type', kind), ('Content-Length', str(len(payload)))])
            return [b'' if method == 'HEAD' else payload]

        def authorized(secret):
            header = env.get('HTTP_AUTHORIZATION', '')
            return hmac.compare_digest(header.encode(), ('Bearer ' + secret).encode())

        try:
            if route == '/health' and method == 'GET':
                return respond('200 OK', {'ok': True})
            pixel = re.fullmatch(r'/p/(' + TOKEN + r')\.gif', route)
            if pixel and method in ('GET', 'HEAD'):
                if method == 'GET':
                    with connect() as db:
                        db.execute('UPDATE tracks SET opens=opens+1, first_open=COALESCE(first_open,?),last_open=? WHERE id=? AND active=1 AND expires>?', (now, now, pixel[1], now))
                # Same response for unknown, inactive, expired and deleted tokens.
                return respond('200 OK', GIF, 'image/gif')
            if route == '/api/tracks' and method == 'POST':
                if not authorized(api_key):
                    return respond('401 Unauthorized', {'error': 'Unauthorized'})
                ident, secret = secrets.token_urlsafe(24), secrets.token_urlsafe(32)
                expires = now + int(ttl_days) * 86400
                with connect() as db:
                    db.execute('DELETE FROM tracks WHERE expires<=?', (now,))
                    if db.execute('SELECT COUNT(*) FROM tracks').fetchone()[0] >= 10000:
                        return respond('429 Too Many Requests', {'error': 'Tracker capacity reached'})
                    db.execute('INSERT INTO tracks(id,secret_hash,created,expires) VALUES(?,?,?,?)', (ident, hashlib.sha256(secret.encode()).hexdigest(), now, expires))
                return respond('201 Created', {'id': ident, 'manage_token': secret, 'pixel_url': origin + '/p/' + ident + '.gif', 'created': now, 'expires': expires, 'active': False})
            match = re.fullmatch(r'/api/tracks/(' + TOKEN + r')(/arm|/disable)?', route)
            if match:
                with connect() as db:
                    row = db.execute('SELECT * FROM tracks WHERE id=?', (match[1],)).fetchone()
                    supplied = env.get('HTTP_AUTHORIZATION', '')
                    supplied = supplied[7:] if supplied.startswith('Bearer ') else ''
                    if not row or not hmac.compare_digest(hashlib.sha256(supplied.encode()).hexdigest(), row['secret_hash']):
                        return respond('404 Not Found', {'error': 'Not found'})
                    if method == 'GET' and not match[2]:
                        return respond('200 OK', {'id': row['id'], 'active': bool(row['active']) and row['expires'] > now, 'expired': row['expires'] <= now, 'image_requests': row['opens'], 'first_request': row['first_open'], 'last_request': row['last_open'], 'created': row['created'], 'expires': row['expires']})
                    if method == 'POST' and match[2]:
                        if row['expires'] <= now:
                            return respond('410 Gone', {'error': 'Expired'})
                        active = 1 if match[2] == '/arm' else 0
                        db.execute('UPDATE tracks SET active=? WHERE id=?', (active, row['id']))
                        return respond('200 OK', {'active': bool(active)})
                    if method == 'DELETE' and not match[2]:
                        db.execute('DELETE FROM tracks WHERE id=?', (row['id'],))
                        return respond('200 OK', {'deleted': True})
            return respond('404 Not Found', {'error': 'Not found'})
        except sqlite3.Error:
            return respond('503 Service Unavailable', {'error': 'Storage unavailable'})
    return app


def from_env():
    return create_app(os.environ.get('TRACKER_DB', '/data/tracker.sqlite3'), os.environ.get('TRACKER_API_KEY', ''), os.environ.get('PUBLIC_BASE_URL', ''), int(os.environ.get('TRACKER_TTL_DAYS', '30')))
