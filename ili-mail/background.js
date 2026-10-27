chrome.runtime.onMessage.addListener((m,s,reply)=>{
 if(s.id!==chrome.runtime.id)return;
 if(m.type==='studio'){chrome.tabs.create({url:chrome.runtime.getURL('studio.html')});reply({ok:true});}
 if(m.type==='stop'){chrome.tts.stop();reply({ok:true});}
 if(m.type==='speak' && typeof m.text==='string'){
 chrome.storage.local.get({language:'he-IL',rate:1},o=>chrome.tts.getVoices(voices=>{
 const voice=voices.find(v=>v.lang?.toLowerCase().startsWith(o.language.split('-')[0]) && !v.remote);
 if(!voice){reply({ok:false,error:'לא נמצא קול מקומי לשפה הנבחרת. התקיני קול מתאים במערכת או החליפי שפה בהגדרות.'});return;}
 chrome.tts.stop();chrome.tts.speak(m.text.slice(0,20000),{voiceName:voice.voiceName,lang:o.language,rate:Number(o.rate)},()=>reply(chrome.runtime.lastError?{ok:false,error:chrome.runtime.lastError.message}:{ok:true}));
 }));return true;}
});