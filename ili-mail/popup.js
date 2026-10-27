const $=id=>document.getElementById(id);
const defaults={language:'he-IL',rate:1,theme:'original',density:'original',rounded:false,chatFont:16};
function show(o){for(const k of Object.keys(defaults)){if(k==='rounded')$(k).checked=o[k];else $(k).value=o[k];}}
chrome.storage.local.get(defaults,show);
for(const k of Object.keys(defaults))$(k).onchange=()=>{const value=k==='rounded'?$(k).checked:['rate','chatFont'].includes(k)?Number($(k).value):$(k).value;chrome.storage.local.set({[k]:value},()=>{$('status').textContent='ההגדרות נשמרו ומוחלות על Gmail פתוח';});};
$('reset').onclick=()=>chrome.storage.local.set({theme:'original',density:'original',rounded:false,chatFont:16},()=>{chrome.storage.local.get(defaults,show);$('status').textContent='עיצוב Gmail אופס';});
$('gmail').onclick=()=>chrome.tabs.create({url:'https://mail.google.com/'});$('studio').onclick=()=>chrome.tabs.create({url:chrome.runtime.getURL('studio.html')});
