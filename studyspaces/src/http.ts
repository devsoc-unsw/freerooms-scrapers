export async function fetchText(url: string, tries = 3): Promise<string> {
    for (let i = 1; ; i++) {
        const res = await fetch(url, {
            headers: { "User-Agent": "freerooms-studyspaces-scraper" },
        });

        if (res.ok) return res.text();
        if (i >= tries) throw new Error(`GET ${url} ${res.status}`);
        await new Promise(r => setTimeout(r, 250 * i));
    }
}