const {contextBridge,ipcRenderer}=require('electron');
contextBridge.exposeInMainWorld('studio',{
 downloadAsio:()=>ipcRenderer.invoke('ui:asio-download'),
 setLanguage:language=>ipcRenderer.invoke('ui:language',language),
 command:(op,data)=>ipcRenderer.invoke('audio:command',op,data),
 slot:(index,action,value)=>ipcRenderer.invoke('audio:slot',index,action,value),
 record:on=>ipcRenderer.invoke('audio:record',on),
 project:action=>ipcRenderer.invoke('audio:project',action),
 exportLog:()=>ipcRenderer.invoke('audio:log'),
 onEvent:fn=>{const listener=(_,data)=>fn(data);ipcRenderer.on('audio:event',listener);return ()=>ipcRenderer.removeListener('audio:event',listener);}
});
