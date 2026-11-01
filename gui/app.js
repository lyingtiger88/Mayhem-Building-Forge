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
 const brief={};for(const k of fields)brief[k]=$(k).type==="number"?Number($(k).value):$(k).value;
 const blob=new Blob([JSON.stringify(brief,null,2)],{type:"application/json"});
 const url=URL.createObjectURL(blob);const link=document.createElement("a");link.href=url;link.download="mayhem-building-brief.json";link.click();URL.revokeObjectURL(url);
});
$("importBrief").addEventListener("click",()=>$("briefFile").click());
$("briefFile").addEventListener("change",async()=>{
 const file=$("briefFile").files[0];if(!file)return;
 try{const brief=JSON.parse(await file.text());for(const k of fields)if(brief[k]!==undefined)$(k).value=brief[k];updateSiteSummary();$("status").textContent="Building brief imported";}catch(err){$("status").textContent="Invalid brief JSON: "+err.message;}
 $("briefFile").value="";
});
function addSvgDownload(card,svg,filename){
 const link=document.createElement("a");link.textContent="Download SVG";link.download=filename;link.href=URL.createObjectURL(new Blob([svg],{type:"image/svg+xml"}));link.style.display="inline-block";link.style.margin="0 12px 12px";link.style.color="#9fc2ff";link.style.fontWeight="700";card.append(link);
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
    const preview=document.createElement("div");preview.className="preview";const img=document.createElement("img");img.alt="Topology-derived floor plan "+plan.profile;img.src=URL.createObjectURL(new Blob([plan.svg],{type:"image/svg+xml"}));preview.append(img);card.append(preview);addSvgDownload(card,plan.svg,"mayhem-"+plan.profile.toLowerCase().replace(/[^a-z0-9]+/g,"-")+".svg");
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
