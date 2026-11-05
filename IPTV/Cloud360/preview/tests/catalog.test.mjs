import { test } from "node:test";
import assert from "node:assert/strict";
import {parseM3U, attrsFromExtinf, inferType, streamFormat, parseXmltvDate, normalizeXtream, playableUrl} from "../catalog.mjs";
test("M3U carrega logos, ids, grupos e urls", () => {
 const sample = '#EXTM3U\n#EXTINF:-1 tvg-logo="https://images.example.org/logo.png" tvg-id="news.br" group-title="Notícias",Notícias\nhttps://video.example.org/stream.m3u8\n';
 const r = parseM3U(sample);
 assert.equal(r.entries.length, 1);
 assert.equal(r.entries[0].logo, "https://images.example.org/logo.png");
 assert.equal(r.entries[0].tvgId, "news.br");
 assert.equal(r.entries[0].group, "Notícias");
 assert.equal(streamFormat(r.entries[0].url), "HLS");
});
test("URLs perigosas são recusadas e repetidas não duplicam",()=>{
 const sample = '#EXTM3U\n#EXTINF:-1,OK\nhttps://example.org/a.m3u8\n#EXTINF:-1,Repeated\nhttps://example.org/a.m3u8\n#EXTINF:-1,Invalid\njavascript:alert(1)\n';
 const r=parseM3U(sample);
 assert.equal(r.entries.length,1); assert.equal(r.duplicates,1); assert.equal(r.invalid,1);
 assert.equal(playableUrl("javascript:alert(1)"),"");
});
test("classifica filme, série e live",()=>{
 assert.equal(inferType({kind:"series",group:""},),"series");
 assert.equal(inferType({group:"Filmes UHD",url:"x"}),"movies");
 assert.equal(inferType({group:"CANAIS: FILMES E SÉRIES",url:"https://example.org/channel.ts"}),"live");
 assert.equal(inferType({group:"Esportes",url:"https://example.org/tv.ts"}),"live");
});
test("data XMLTV usa fuso -0300",()=>{
 assert.equal(new Date(parseXmltvDate("20261105190000 -0300")).toISOString(),"2026-11-05T22:00:00.000Z");
});
test("Xtream normaliza com categorias e imagens",()=>{
 const list=normalizeXtream({server:"https://provider.example.org",username:"demo",password:"demo",categories:[{category_id:"7",category_name:"Documentários"}],channels:[{stream_id:42,name:"Natureza",category_id:"7",stream_icon:"https://example.org/logo.jpg"}]});
 assert.equal(list.length,1);assert.equal(list[0].group,"Documentários");
 assert.equal(list[0].logo,"https://example.org/logo.jpg");
 assert.equal(list[0].url,"https://provider.example.org/live/demo/demo/42.ts");
});
