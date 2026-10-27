const $=id=>document.getElementById(id);let recorder,stream,ticker,url,start,chunks=[],pending=false;
function cleanup(){clearInterval(ticker);stream?.getTracks().forEach(t=>t.stop());stream=null;}
$('record').onclick=async()=>{if(pending||recorder?.state==='recording')return;pending=true;$('record').disabled=true;try{
 stream=await navigator.mediaDevices.getUserMedia({audio:true});chunks=[];
 const mime=['audio/webm;codecs=opus','audio/webm','audio/mp4'].find(t=>MediaRecorder.isTypeSupported(t));recorder=new MediaRecorder(stream,mime?{mimeType:mime}:{});
 recorder.ondataavailable=e=>{if(e.data.size)chunks.push(e.data);};
 recorder.onstop=()=>{cleanup();if(url)URL.revokeObjectURL(url);const type=recorder.mimeType||'audio/webm';url=URL.createObjectURL(new Blob(chunks,{type}));$('audio').src=url;$('audio').hidden=false;$('download').href=url;$('download').download='ili-voice-'+new Date().toISOString().replace(/[:.]/g,'-')+(type.includes('mp4')?'.m4a':'.webm');$('download').hidden=false;$('record').disabled=false;$('stop').disabled=true;$('status').textContent='ההקלטה מוכנה להאזנה ושמירה.';};
 recorder.onerror=()=>{cleanup();$('record').disabled=false;$('stop').disabled=true;$('status').textContent='אירעה שגיאה בהקלטה. נסי שוב.';};
 $('audio').pause();$('audio').hidden=true;$('download').hidden=true;recorder.start();start=Date.now();$('stop').disabled=false;$('status').textContent='מקליטה…';ticker=setInterval(()=>{const n=Math.floor((Date.now()-start)/1000);$('timer').textContent=String(Math.floor(n/60)).padStart(2,'0')+':'+String(n%60).padStart(2,'0');if(n>=300&&recorder.state==='recording')recorder.stop();},250);
 }catch(e){cleanup();$('record').disabled=false;$('status').textContent=e.name==='NotAllowedError'?'הרשאת המיקרופון נדחתה. אפשרי גישה בהגדרות הדפדפן ונסי שוב.':'לא ניתן להפעיל מיקרופון: '+e.message;}finally{pending=false;}};
$('stop').onclick=()=>{if(recorder?.state==='recording'){recorder.stop();$('stop').disabled=true;}};
window.addEventListener('beforeunload',e=>{if(pending||recorder?.state==='recording'){e.preventDefault();e.returnValue='';}});window.addEventListener('pagehide',cleanup);
