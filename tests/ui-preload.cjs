// UI smoke-test bridge only; the production build uses electron/preload.cjs.
const {contextBridge,ipcRenderer}=require('electron');
let state={},listener;const changes=[];
contextBridge.exposeInMainWorld('studio',{
 command:async(op,data={})=>{changes.push({op,data});if(op==='devices')return {inputs:['Line (XOX K10)'],outputs:['Speakers (XOX K10)'],drivers:['ASIO Link Pro']};if(op==='ports')return {inputs:['In 1','In 2','LinkIn3','LinkIn4'],outputs:['Out 1','Out 2','LinkOut3','LinkOut4']};if(op==='configure'||op==='params'){state={...state,...data};return state;}if(op==='live')return {live:data.on,source:'ASIO input ports'};if(op==='test')return {output:state.driver,duration:2};if(op==='auto-vocal')return {running:!data.cancel};if(op==='auto-amount'){state.autoAmount=data.autoAmount;return state;}if(op==='control-panel')return {opened:true};throw Error(op);},
 slot:async(i,action,value)=>({path:action==='remove'?'':'Test Vocal.vst3',on:action==='on'?value:true,bypass:action==='bypass'?value:false,parameters:[{name:'Dry / Wet',value:.5}]}),
 record:async(on)=>({recording:on}),project:async()=>({...state,slots:Array(4).fill({path:'',on:true,bypass:false})}),exportLog:async()=>true,
 onEvent:fn=>{listener=fn;setTimeout(()=>fn({event:'ready',backend:'UI test bridge'}),50);}
});
contextBridge.exposeInMainWorld('testBridge',{changes:()=>changes,emit:data=>listener?.(data)});
