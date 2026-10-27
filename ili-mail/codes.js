/* Local, user-triggered extraction. No network calls and no persistent storage. */
(()=>{
 'use strict';
 function extract(text){
  if(typeof text!=='string')return [];
  const input=text.slice(0,200000),results=[],seen=new Set();
  const contextPattern=/(?:verification\s+code|security\s+code|one[ -]time\s+(?:code|password)|sign[ -]?in\s+code|login\s+code|\bOTP\b|קוד\s+(?:האימות|אימות|האבטחה|אבטחה|הכניסה|כניסה|חד[\s־-]?פעמי)|הקוד\s+שלך)/i;
  for(const m of input.matchAll(/\b(?:\d{3}[ -]\d{3}|\d{4,8})\b/g)){
   const start=m.index,end=start+m[0].length;
   // Ignore a fragment of a date, telephone number, or a longer numeric token.
   if(/[\d:]$/.test(input.slice(0,start))||/^[\d:]/.test(input.slice(end))||/\d[-/.]$/.test(input.slice(0,start))||/^[-/.]\d/.test(input.slice(end)))continue;
   const context=input.slice(Math.max(0,start-80),Math.min(input.length,end+80)).replace(/\s+/g,' ').trim();
   if(!contextPattern.test(context))continue;
   const code=m[0].replace(/[ -]/g,'');if(seen.has(code))continue;
   seen.add(code);results.push({code,context});if(results.length>=10)break;
  }
  return results;
 }
 globalThis.IliMailCodes=Object.freeze({extract});
 if(typeof module==='object'&&module.exports)module.exports={extract};
})();
