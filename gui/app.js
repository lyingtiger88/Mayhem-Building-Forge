const $=id=>document.getElementById(id);
const fields=["width","depth","south","north","east","west","entrance","floors","bedrooms","bathrooms","living_area","kitchen_area","bedroom_area","bathroom_area","hallway_area","seed","profile"];
const defaults={width:20,depth:15,south:3,north:2,east:1,west:2,entrance:"south",floors:1,bedrooms:1,bathrooms:1,living_area:24,kitchen_area:12,bedroom_area:14,bathroom_area:6,hallway_area:8,seed:42,profile:"all"};
function updateSiteSummary(){
 const width=Number($("width").value)-Number($("east").value)-Number($("west").value);
 const depth=Number($("depth").value)-Number($("south").value)-Number($("north").value);
 const host=$("siteSummary");
 if(width<=0||depth<=0||!Number.isFinite(width)||!Number.isFinite(depth)){
  host.textContent="Invalid setbacks: no buildable footprint remains.";host.style.color="#ff8585";return;
 }
 const buildable=width*depth;
 const target=Number($("living_area").value)+Number($("kitchen_area").value)+
  Number($("bedrooms").value)*Number($("bedroom_area").value)+
  Number($("bathrooms").value)*Number($("bathroom_area").value)+Number($("hallway_area").value);
 const remaining=buildable-target;
 host.textContent="Buildable: "+width.toFixed(1)+" × "+depth.toFixed(1)+" m · "+buildable.toFixed(1)+" m² | Target area total: "+target.toFixed(1)+" m² | "+(remaining>=0?"Target gap: "+remaining.toFixed(1)+" m²":"Targets exceed footprint by: "+Math.abs(remaining).toFixed(1)+" m²");
 host.style.color=remaining<0?"#ff8585":"var(--muted)";
}
for(const id of ["width","depth","south","north","east","west","bedrooms","bathrooms","living_area","kitchen_area","bedroom_area","bathroom_area","hallway_area"])$(id).addEventListener("input",updateSiteSummary);
$("reset").addEventListener("click",()=>{for(const k of fields)$(k).value=defaults[k];updateSiteSummary()});
updateSiteSummary();
$("exportBrief").addEventListener("click",()=>{
 const brief={schema_version:1};for(const k of fields)brief[k]=$(k).type==="number"?Number($(k).value):$(k).value;
 const blob=new Blob([JSON.stringify(brief,null,2)],{type:"application/json"});
 const url=URL.createObjectURL(blob);const link=document.createElement("a");link.href=url;link.download="mayhem-building-brief.json";link.click();URL.revokeObjectURL(url);
});
$("importBrief").addEventListener("click",()=>$("briefFile").click());
$("briefFile").addEventListener("change",async()=>{
 const file=$("briefFile").files[0];if(!file)return;
 try{const brief=JSON.parse(await file.text());if(brief.schema_version!==undefined&&brief.schema_version!==1)throw new Error("Unsupported brief schema version");for(const k of fields)if(brief[k]!==undefined)$(k).value=brief[k];updateSiteSummary();$("status").textContent="Building brief imported";}catch(err){$("status").textContent="Invalid brief JSON: "+err.message;}
 $("briefFile").value="";
});
function addSvgDownload(card,svg,filename){
 const link=document.createElement("a");link.textContent="Download SVG";link.download=filename;link.href=URL.createObjectURL(new Blob([svg],{type:"image/svg+xml"}));link.style.display="inline-block";link.style.margin="0 12px 12px";link.style.color="#9fc2ff";link.style.fontWeight="700";card.append(link);
}

function mountFurnitureEditor(card,preview,svgText,filename){
 const doc=new DOMParser().parseFromString(svgText,"image/svg+xml");
 const root=doc.documentElement;
 if(root.localName!=="svg"||doc.querySelector("parsererror"))throw new Error("Generated SVG could not be opened for editing");
 root.setAttribute("width","100%");root.removeAttribute("height");root.style.display="block";
 preview.append(document.importNode(root,true));
 const svg=preview.querySelector("svg");
 const items=[...svg.querySelectorAll("#FURNITURE [data-furniture-id]")];
 const initial=items.map(el=>({x:el.dataset.x,y:el.dataset.y,rotation:el.dataset.rotation}));
 const toolbar=document.createElement("div");toolbar.className="furniture-toolbar";
 const edit=document.createElement("button");edit.type="button";edit.className="secondary";edit.textContent="Enable furniture editing";
 const rotate=document.createElement("button");rotate.type="button";rotate.className="secondary";rotate.textContent="Rotate 90°";rotate.disabled=true;
 const zoomOut=document.createElement("button");zoomOut.type="button";zoomOut.className="secondary";zoomOut.textContent="−";
 const zoomLabel=document.createElement("span");zoomLabel.className="zoom-label";zoomLabel.textContent="100%";
 const zoomIn=document.createElement("button");zoomIn.type="button";zoomIn.className="secondary";zoomIn.textContent="+";
 const fit=document.createElement("button");fit.type="button";fit.className="secondary";fit.textContent="Fit";
 const fullscreen=document.createElement("button");fullscreen.type="button";fullscreen.className="secondary";fullscreen.textContent="Expand editor";
 const reset=document.createElement("button");reset.type="button";reset.className="secondary";reset.textContent="Reset layout";
 const download=document.createElement("button");download.type="button";download.textContent="Download edited SVG";
 const status=document.createElement("small");status.textContent=items.length+" furniture objects · select and drag to move";
 toolbar.append(edit,rotate,zoomOut,zoomLabel,zoomIn,fit,fullscreen,reset,download,status);card.insertBefore(toolbar,preview);
 card.classList.add("furniture-editor-card");
 let enabled=false,selected=null,drag=null;
 const originalView=svg.viewBox.baseVal;
 const base={x:originalView.x,y:originalView.y,width:originalView.width,height:originalView.height};
 let zoom=1;
 const applyZoom=next=>{
  const value=Math.max(.65,Math.min(3,next));const cx=base.x+base.width/2,cy=base.y+base.height/2;
  zoom=value;const w=base.width/zoom,h=base.height/zoom;
  svg.setAttribute("viewBox",(cx-w/2)+" "+(cy-h/2)+" "+w+" "+h);zoomLabel.textContent=Math.round(zoom*100)+"%";
 };
 const setTransform=el=>{
  const x=Number(el.dataset.x),y=Number(el.dataset.y),w=Number(el.dataset.width),h=Number(el.dataset.depth),r=Number(el.dataset.rotation||0);
  el.setAttribute("transform","translate("+(x+w/2)+" "+(y+h/2)+") rotate(" +r+") translate("+(-w/2)+" "+(-h/2)+")");
 };
 const bounds=el=>{
  const x=Number(el.dataset.x),y=Number(el.dataset.y),w=Number(el.dataset.width),h=Number(el.dataset.depth),r=((Number(el.dataset.rotation||0)%180)+180)%180;
  return r===90?{x:x+(w-h)/2,y:y+(h-w)/2,w:h,h:w}:{x,y,w,h};
 };
 const collision=el=>{
  const a=bounds(el);
  return items.some(other=>{if(other===el||other.dataset.roomId!==el.dataset.roomId)return false;const b=bounds(other),g=.18;
   return a.x-g<b.x+b.w+g&&a.x+a.w+g>b.x-g&&a.y-g<b.y+b.h+g&&a.y+a.h+g>b.y-g;});
 };
 const furnitureLayer=svg.querySelector("#FURNITURE");
 const roomBoundaries=new Map();
 svg.querySelectorAll("#ROOM_BOUNDARIES [data-room-boundary-id]").forEach(group=>{
  const rings=[...group.querySelectorAll("polygon")].map(poly=>poly.getAttribute("points").trim().split(/\\s+/).map(pair=>pair.split(",").map(Number)));
  roomBoundaries.set(group.dataset.roomBoundaryId,rings);
 });
 const pointInRing=(p,ring)=>{
  let inside=false;
  for(let i=0,j=ring.length-1;i<ring.length;j=i++){
   const a=ring[i],b=ring[j];
   if(((a[1]>p.y)!==(b[1]>p.y))&&(p.x<(b[0]-a[0])*(p.y-a[1])/(b[1]-a[1])+a[0]))inside=!inside;
  }
  return inside;
 };
 const withinRoom=(el,x=Number(el.dataset.x),y=Number(el.dataset.y),rotation=Number(el.dataset.rotation||0))=>{
  const rings=roomBoundaries.get(el.dataset.roomId);if(!rings||!rings.length)return false;
  const w=Number(el.dataset.width),h=Number(el.dataset.depth),cx=x+w/2,cy=y+h/2,rad=rotation*Math.PI/180;
  const local=[[-w/2,-h/2],[w/2,-h/2],[w/2,h/2],[-w/2,h/2],[0,-h/2],[w/2,0],[0,h/2],[-w/2,0],[0,0]];
  const samples=local.map(([lx,ly])=>({x:cx+lx*Math.cos(rad)-ly*Math.sin(rad),y:cy+lx*Math.sin(rad)+ly*Math.cos(rad)}));
  return samples.every(p=>rings.reduce((inside,ring)=>inside!==pointInRing(p,ring),false));
 };
 const point=e=>{
  const matrix=furnitureLayer.getScreenCTM();
  if(!matrix)throw new Error("Furniture coordinate transform is unavailable");
  const p=svg.createSVGPoint();p.x=e.clientX;p.y=e.clientY;
  return p.matrixTransform(matrix.inverse());
 };
 const select=el=>{if(selected)selected.style.filter="";selected=el;if(selected)selected.style.filter="drop-shadow(0 0 3px #f59e0b)";rotate.disabled=!selected;};
 edit.addEventListener("click",()=>{enabled=!enabled;edit.textContent=enabled?"Finish editing":"Enable furniture editing";status.textContent=enabled?"Drag furniture smoothly; zoom or expand for precision.":"Editing paused.";svg.style.cursor=enabled?"grab":"default";});
 rotate.addEventListener("click",()=>{if(!selected)return;const old=selected.dataset.rotation;selected.dataset.rotation=String((Number(old||0)+90)%360);setTransform(selected);if(!withinRoom(selected)||collision(selected)){selected.dataset.rotation=old;setTransform(selected);status.textContent="Rotation rejected: object would cross its room boundary or overlap another object.";}else status.textContent="Furniture rotated 90°.";});
 zoomIn.addEventListener("click",()=>applyZoom(zoom*1.2));zoomOut.addEventListener("click",()=>applyZoom(zoom/1.2));fit.addEventListener("click",()=>applyZoom(1));
 fullscreen.addEventListener("click",()=>{const open=!card.classList.contains("furniture-editor-fullscreen");card.classList.toggle("furniture-editor-fullscreen",open);document.body.classList.toggle("furniture-editor-open",open);fullscreen.textContent=open?"Close expanded editor":"Expand editor";if(open)applyZoom(1);});
 reset.addEventListener("click",()=>{items.forEach((el,i)=>{el.dataset.x=initial[i].x;el.dataset.y=initial[i].y;el.dataset.rotation=initial[i].rotation;setTransform(el);});status.textContent="Original generated layout restored.";});
 download.addEventListener("click",()=>{const output=new XMLSerializer().serializeToString(svg);const url=URL.createObjectURL(new Blob([output],{type:"image/svg+xml"}));const a=document.createElement("a");a.href=url;a.download=filename.replace(".svg","-edited.svg");a.click();URL.revokeObjectURL(url);});
 items.forEach(el=>{el.style.cursor="grab";el.addEventListener("pointerdown",e=>{
  if(!enabled||el.dataset.movable!=="true")return;e.preventDefault();select(el);
  const p=point(e);drag={el,pointerId:e.pointerId,startX:p.x,startY:p.y,x:Number(el.dataset.x),y:Number(el.dataset.y)};
  el.setPointerCapture(e.pointerId);el.style.cursor="grabbing";svg.style.cursor="grabbing";
 });});
 svg.addEventListener("pointermove",e=>{
  if(!drag||e.pointerId!==drag.pointerId)return;
  const p=point(e),el=drag.el,previousX=el.dataset.x,previousY=el.dataset.y;
  const nextX=drag.x+(p.x-drag.startX),nextY=drag.y+(p.y-drag.startY);
  el.dataset.x=String(nextX);el.dataset.y=String(nextY);
  if(!withinRoom(el)||collision(el)){
   el.dataset.x=previousX;el.dataset.y=previousY;
   status.textContent=!withinRoom(el,nextX,nextY)?"Movement limited to the current room.":"Movement blocked by another furniture object.";
  }else{setTransform(el);status.textContent="Furniture position updated.";}

 });
 const finishDrag=(e,cancelled=false)=>{
  if(!drag||e.pointerId!==drag.pointerId)return;const el=drag.el;
  if(cancelled||!withinRoom(el)||collision(el)){el.dataset.x=String(drag.x);el.dataset.y=String(drag.y);setTransform(el);status.textContent=cancelled?"Move cancelled.":"Move rejected: furniture must stay inside its room without overlaps.";}
  else status.textContent="Furniture position updated.";
  el.style.cursor="grab";svg.style.cursor=enabled?"grab":"default";if(el.hasPointerCapture(e.pointerId))el.releasePointerCapture(e.pointerId);drag=null;
 };
 svg.addEventListener("pointerup",e=>finishDrag(e));svg.addEventListener("pointercancel",e=>finishDrag(e,true));
 svg.addEventListener("wheel",e=>{if(!e.ctrlKey&&!e.metaKey)return;e.preventDefault();applyZoom(zoom*(e.deltaY<0?1.12:1/1.12));},{passive:false});
}


$("brief").addEventListener("submit",async e=>{
 e.preventDefault();const button=$("generate");button.disabled=true;$("status").textContent="Generating with native C++ engine…";
 $("results").className="empty";$("results").textContent="Running geometry generation and validation…";
 const brief={};for(const k of fields){brief[k]=$(k).type==="number"?Number($(k).value):$(k).value}
 try{
  const response=await fetch("/api/generate",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(brief)});
  const payload=await response.json();const host=$("results");host.className="results";host.innerHTML="";
  if(!payload.candidates||payload.candidates.length===0){host.className="empty";const b=document.createElement("b");b.textContent="No valid candidate returned";host.append(b,document.createTextNode(payload.error||"The current planner could not satisfy this brief."));$("status").textContent="Generation failed";return}
  let success=0;
  for(const candidate of payload.candidates){
   const card=document.createElement("article");card.className="result";
   const head=document.createElement("div");head.className="result-head";const name=document.createElement("b");name.textContent=candidate.profile;const tag=document.createElement("span");tag.textContent=candidate.ok?"VALIDATED BY CURRENT CHECKS":"NOT GENERATED";head.append(name,tag);card.append(head);
   if(candidate.svg){const preview=document.createElement("div");preview.className="preview";const img=document.createElement("img");img.alt="Generated floor plan "+candidate.profile;img.src=URL.createObjectURL(new Blob([candidate.svg],{type:"image/svg+xml"}));preview.append(img);card.append(preview);addSvgDownload(card,candidate.svg,"mayhem-legacy-"+candidate.profile+".svg");success++}
   const detail=document.createElement("div");detail.className="details";detail.textContent=candidate.stdout||candidate.error||"";card.append(detail);host.append(card);
  }
  const topologyHost=$("topologyPreview");topologyHost.className="empty";topologyHost.innerHTML="";
  if(payload.topology_svg){
   topologyHost.className="";topologyHost.style.padding="14px";
   const graphImage=document.createElement("img");graphImage.alt="Alternative room adjacency graph candidates";graphImage.style.width="100%";graphImage.style.background="#f8fafc";graphImage.style.borderRadius="8px";
   graphImage.src=URL.createObjectURL(new Blob([payload.topology_svg],{type:"image/svg+xml"}));topologyHost.append(graphImage);
   const caption=document.createElement("div");caption.className="details";caption.textContent=payload.topology_report||"Topology graph candidates generated.";topologyHost.append(caption);
  }else{topologyHost.textContent=payload.topology_error||"Topology graph preview unavailable."}
  const plansHost=$("topologyPlans");plansHost.className="empty";plansHost.innerHTML="";
  if(payload.topology_plans&&payload.topology_plans.length){
   plansHost.className="results";
   for(const plan of payload.topology_plans){
    const card=document.createElement("article");card.className="result";
    const head=document.createElement("div");head.className="result-head";const name=document.createElement("b");name.textContent=plan.profile;const tag=document.createElement("span");tag.textContent="RANK #"+(plan.rank||"?")+" · TOPOLOGY SOLVED";head.append(name,tag);card.append(head);
    const preview=document.createElement("div");preview.className="preview";card.append(preview);
    const planFilename="mayhem-"+plan.profile.toLowerCase().replace(/[^a-z0-9]+/g,"-")+".svg";
    mountFurnitureEditor(card,preview,plan.svg,planFilename);
    if(plan.quality&&Object.keys(plan.quality).length){const metrics=document.createElement("div");metrics.className="metrics";for(const [key,label] of [["daylight","Daylight potential"],["privacy","Privacy"],["circulation","Circulation"],["overall","Overall quality"],["doors","Doors"],["windows","Windows"],["openings","Openings"],["open-plan","Open-plan links"],["accessible","Rooms accessible"]]){if(plan.quality[key]===undefined)continue;const item=document.createElement("div");item.className="metric";const caption=document.createElement("small");caption.textContent=label;const value=document.createElement("b");value.textContent=typeof plan.quality[key]==="number"?(key==="doors"||key==="windows"||key==="openings"||key==="open-plan"?String(Math.round(plan.quality[key])):Math.round(plan.quality[key])+"/100"):String(plan.quality[key]);item.append(caption,value);metrics.append(item)}card.append(metrics)}
    if(plan.warnings&&plan.warnings.length){const warnings=document.createElement("div");warnings.className="details";warnings.textContent="Diagnostics:\n• "+plan.warnings.join("\n• ");card.append(warnings)}
    plansHost.append(card);
   }
   const report=document.createElement("pre");report.className="details";report.style.gridColumn="1/-1";report.textContent=payload.topology_plan_report||"";plansHost.append(report);
  }else{plansHost.textContent=payload.topology_plan_error||"No topology graph could be realized as floor-plan geometry for this brief."}
  $("status").textContent=success+" candidate(s) generated";
 }catch(err){$("results").className="empty";$("results").textContent="Could not contact local generation service: "+err.message;$("status").textContent="Local service unavailable"}
 finally{button.disabled=false}
});
