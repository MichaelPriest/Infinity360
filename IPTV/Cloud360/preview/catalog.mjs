/*
 * Cloud360 portable catalog logic.
 * Stream URLs are metadata, not proof of playback support or authorization.
 */
export const MAX_ITEMS = 12000;
const scheme = /^https?:\/\//i;
export function playableUrl(input, base) {
  const raw = String(input ?? "").trim();
  if (!raw) return "";
  try {
    const url = new URL(raw, base || undefined);
    if (!/^https?:$/.test(url.protocol) || url.username || url.password) return "";
    return url.href;
  } catch { return ""; }
}
export function attrsFromExtinf(line) {
  const attributes = Object.create(null);
  const start = line.indexOf(" ");
  const end = extinfDelimiter(line);
  const region = line.substring(start < 0 ? 0 : start, end < 0 ? line.length : end);
  const regexp = /([\w-]+)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s]+))/g;
  for (const match of region.matchAll(regexp))
    attributes[match[1].toLowerCase()] = match[2] ?? match[3] ?? match[4] ?? "";
  return attributes;
}
function extinfDelimiter(line, last = false) {
  let quote = "", comma = -1;
  for (let i = 0; i < line.length; i++) {
    const c = line[i];
    if (quote) { if (c === quote) quote = ""; }
    else if (c === '"' || c === "'") quote = c;
    else if (c === ",") { if (!last) return i; comma = i; }
  }
  return comma;
}
function displayName(line, legacy = false) {
  const comma = extinfDelimiter(line, legacy);
  return comma < 0 ? "Sem nome" : line.slice(comma + 1).trim() || "Sem nome";
}
function stableId(channel, name = channel.name) {
  const value = name + "\x1f" + channel.url;
  let hash = 2166136261;
  for (let i = 0; i < value.length; i++) hash = Math.imul(hash ^ value.charCodeAt(i), 16777619);
  return (hash >>> 0).toString(16);
}
export function inferType(item) {
  const group = (item.group || "").toLocaleLowerCase("pt-BR");
  if (item.kind === "series") return "series";
  if (item.kind === "live" || /^(canais?|tv)(\b|\s*:)/i.test(group)) return "live";
  if (/s[eé]ries?|temporada|season/.test(group)) return "series";
  if (item.kind === "movie" || /filmes?|movie|vod|cinema|animes?|desenhos?/.test(group) || /\.(mp4|mkv)(?:[?#]|$)/i.test(item.url))
    return "movies";
  return "live";
}
export function streamFormat(url) {
  if (!playableUrl(url)) return "desconhecido";
  const path = new URL(url).pathname.toLowerCase();
  if (path.endsWith(".m3u8")) return "HLS";
  if (path.endsWith(".mpd")) return "DASH";
  if (path.endsWith(".ts")) return "MPEG-TS";
  if (/\.(mp4|m4v|mov)$/.test(path)) return "MP4";
  if (/\.(mp3|aac|m4a)$/.test(path)) return "Áudio";
  if (/\.(mkv|avi)$/.test(path)) return "Contêiner legado";
  return "HTTP/outro";
}
export function parseM3U(text, { baseUrl = "", maxItems = MAX_ITEMS } = {}) {
  const entries = [], seen = new Set();
  let pending = null, legacyName = "", invalid = 0, duplicates = 0;
  for (const raw of String(text).replace(/^\uFEFF/, "").split(/\r?\n/)) {
    const line = raw.trim();
    if (line.startsWith("#EXTINF:")) {
      const a = attrsFromExtinf(line);
      legacyName = displayName(line, true); // Existing history and favorites use this spelling.
      pending = {
        name: displayName(line), group: a["group-title"] || "Outros",
        logo: playableUrl(a["tvg-logo"], baseUrl),
        tvgId: a["tvg-id"] || "", tvgName: a["tvg-name"] || "",
        kind: "", source: "m3u"
      };
    } else if (line.startsWith("#EXTGRP:") && pending) {
      pending.group = line.substring(8).trim() || "Outros";
    } else if (line && !line.startsWith("#") && pending) {
      const url = playableUrl(line, baseUrl);
      if (!url) invalid++;
      else if (seen.has(url)) duplicates++;
      else {
        const item = { ...pending, url };
        item.id = stableId(item, legacyName);
        entries.push(item); seen.add(url);
      }
      pending = null;
      if (entries.length >= maxItems) break;
    }
  }
  return { entries, invalid, duplicates, truncated: entries.length >= maxItems };
}
export function parseXmltvDate(value) {
  const m = /^(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})?\s*([+-])?(\d{2})?(\d{2})?/.exec(String(value || ""));
  if (!m) return NaN;
  const [_, y, mo, d, h, mi, sec, sign, zoneH, zoneM] = m;
  const offset = sign ? ((Number(zoneH) * 60 + Number(zoneM)) * (sign === "+" ? 1 : -1)) : 0;
  return Date.UTC(+y, +mo - 1, +d, +h, +mi, +(sec || 0)) - offset * 60000;
}
export function parseXmltv(xmlText, maxProgrammes = 35000) {
  if (typeof DOMParser === "undefined") throw Error("XMLTV requer um navegador.");
  const xml = new DOMParser().parseFromString(xmlText, "application/xml");
  if (xml.querySelector("parsererror")) throw Error("XMLTV inválido.");
  const icons = new Map(), programmes = new Map();
  for (const el of xml.getElementsByTagName("channel")) {
    const id = el.getAttribute("id") || "";
    const icon = el.getElementsByTagName("icon")[0]?.getAttribute("src") || "";
    if (id && icon) icons.set(id, playableUrl(icon));
  }
  let count = 0;
  for (const el of xml.getElementsByTagName("programme")) {
    if (++count > maxProgrammes) break;
    const id = el.getAttribute("channel") || "";
    const start = parseXmltvDate(el.getAttribute("start"));
    const stop = parseXmltvDate(el.getAttribute("stop"));
    if (!id || !Number.isFinite(start) || !Number.isFinite(stop) || stop <= start) continue;
    const record = { start, stop, title: el.getElementsByTagName("title")[0]?.textContent?.trim() || "Sem título",
      desc: el.getElementsByTagName("desc")[0]?.textContent?.trim() || "" };
    if (!programmes.has(id)) programmes.set(id, []);
    programmes.get(id).push(record);
  }
  for (const list of programmes.values()) list.sort((a, b) => a.start - b.start);
  return { icons, programmes, total: count };
}
export function scheduleFor(item, guide, now = Date.now()) {
  if (!guide) return { now: null, next: null };
  const id = item.tvgId || item.tvgName || "";
  const schedule = guide.programmes.get(id) || guide.programmes.get(item.name) || [];
  const current = schedule.find(p => p.start <= now && p.stop > now) || null;
  const next = schedule.find(p => p.start > now) || null;
  return { now: current, next };
}
export function normalizeXtream({ channels = [], categories = [], kind = "live", server, username, password }) {
  const groups = new Map(categories.map(x => [String(x.category_id), x.category_name]));
  const root = playableUrl(server);
  if (!root) throw Error("Servidor Xtream inválido");
  const base = root.replace(/\/$/, "");
  const result = [];
  for (const x of channels) {
    const id = kind === "series" ? x.series_id : x.stream_id;
    if (!/^\d+$/.test(String(id))) continue;
    const name = String(x.name || "Sem nome");
    const ext = String(x.container_extension || (kind === "live" ? "ts" : "mp4")).toLowerCase();
    const extension = /^(ts|m3u8|mp4|mkv|avi)$/.test(ext) ? ext : "mp4";
    const url = kind === "series" ? "" :
      base + "/" + (kind === "movie" ? "movie" : "live") + "/" +
      encodeURIComponent(username) + "/" + encodeURIComponent(password) + "/" + id + "." + extension;
    const item = { name, group: String(groups.get(String(x.category_id)) || "Outros"),
      logo: playableUrl(x.stream_icon || x.cover || ""), tvgId: String(x.epg_channel_id || ""),
      url, kind, seriesId: kind === "series" ? id : null, source: "xtream" };
    item.id = stableId({ name: name + "/" + kind + "/" + id, url: base });
    result.push(item);
  }
  return result;
}
