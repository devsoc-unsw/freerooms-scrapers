import * as cheerio from "cheerio";
import { BASE, LIST_URL } from "./config.js"
import { fetchText } from "./http.js";

export type ListItem = { name?: string; url: string; capacity?: number };
export type Detail = { title: string; buildingId?: string; coords?: { lat: number, long: number } };

function parseCapacity(text: string): number | undefined {
    const m = text.match(/\bCapacity\s+(\d{1,4})\b/);
    return m ? Number(m[1]) : undefined;
}

export async function parseList(): Promise<ListItem[]> {
    const html = await fetchText(LIST_URL);
    const $ = cheerio.load(html);
    const out: ListItem[] = [];

    $("article.view-mode-teaser").each((_, el) => {
        const $card = $(el);
        const $a = $card.find("h3 a").first();
        const href = String($a.attr("href") || "");
        if (!href) return;

        const url = new URL(href, LIST_URL).toString();
        if (!/\/physical-spaces\/study-spaces\//.test(url)) return;


        const name = $a.text().replace(/Capacity.*$/i, '').trim() || undefined;

        const text = $card.find(".field--name-field-body-text").text().replace(/\s+/g, " ").trim();
        const capacity = parseCapacity(text);

        out.push({ name, url, capacity });
    });

    const seen = new Set<string>();
    return out.filter(i => (seen.has(i.url) ? false : seen.add(i.url)));
}

export async function parseDetail(url: string): Promise<Detail> {
    const html = await fetchText(url);
    const $ = cheerio.load(html);
    const title = ($("h1").first().text() || $("h2").first().text()).trim();
    const body = $("body").text();
    const buildingId = body.match(/Building ID\s+([A-Z]-[A-Z]\d{2})/i)?.[1];
    return { title, buildingId };
}