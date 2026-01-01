import { chromium, Page } from "playwright";
import { normaliseRoomName } from "./nameParsers";

type CategoryEvent = {
    Identity: string;
    Name: string;
};

type FilterResponse = {
    CategoryEvents?: CategoryEvent[];
};

const XHR_HOOK = `
(() => {
  window.__waitForNextCategoryResponse = function () {
    return new Promise(resolve => {
      const originalOpen = XMLHttpRequest.prototype.open;
      const originalSend = XMLHttpRequest.prototype.send;

      XMLHttpRequest.prototype.open = function (method, url, ...rest) {
        // @ts-ignore
        this.__url = url;
        return originalOpen.call(this, method, url, ...rest);
      };

      XMLHttpRequest.prototype.send = function (body) {
        // @ts-ignore
        if (typeof this.__url === "string" && this.__url.includes("/CategoryTypes/Categories/Events/Filter")) {
          this.addEventListener("load", () => {
            try {
              resolve(JSON.parse(this.responseText));
            } catch {
              resolve(null);
            }
          });

          // single-use
          XMLHttpRequest.prototype.open = originalOpen;
          XMLHttpRequest.prototype.send = originalSend;
        }

        return originalSend.call(this, body);
      };
    });
  };

  console.log("Awaitable XHR hook installed");
})();
`;

async function clickBatch(page: Page, indices: number[], clickDelayMs: number) {
    await page.evaluate(
        async ({ indices, clickDelayMs }) => {
            const sleep = (ms: number) => new Promise(r => setTimeout(r, ms));
            const options = Array.from(document.querySelectorAll("mat-list-option"));

            for (const i of indices) {
                const opt = options[i] as HTMLElement | undefined;
                if (!opt) continue;
                opt.scrollIntoView({ block: "center" });
                opt.click();
                await sleep(clickDelayMs);
            }
        },
        { indices, clickDelayMs }
    );
}

async function deselectBatch(page: Page, indices: number[], clickDelayMs: number) {
    await clickBatch(page, indices, clickDelayMs);
}

export async function collectSessionIdentities(
    url: string,
    opts?: {
        batchSize?: number;
        maxRooms?: number;
        clickDelayMs?: number;
    }
): Promise<Map<string, string>> {
    const batchSize = opts?.batchSize ?? 20;
    const maxRooms = opts?.maxRooms ?? Infinity;
    const clickDelayMs = opts?.clickDelayMs ?? 80;

    const browser = await chromium.launch({ headless: false });
    const context = await browser.newContext();
    const page = await context.newPage();

    console.log("Opening NSS page...");
    await page.goto(url, { waitUntil: "domcontentloaded" });

    console.log("Waiting for room list...");
    await page.waitForSelector("mat-list-option", { timeout: 0 });

    await page.addInitScript(XHR_HOOK);
    await page.reload({ waitUntil: "domcontentloaded" });
    await page.waitForSelector("mat-list-option", { timeout: 0 });

    const total = await page.evaluate(() => document.querySelectorAll("mat-list-option").length);
    console.log(`Rooms in list: ${total}`);

    const roomMap = new Map<string, string>();

    let processed = 0;
    while (processed < total && processed < maxRooms) {
        const end = Math.min(processed + batchSize, total, maxRooms);
        const batchIndices = Array.from({ length: end - processed }, (_, k) => processed + k);

        console.log(`\nBatch ${processed + 1} → ${end}`);

        await clickBatch(page, batchIndices, clickDelayMs);

        console.log("Waiting for batch response...");
        const response: FilterResponse | null = await page.evaluate(() => {
            // @ts-ignore
            return window.__waitForNextCategoryResponse();
        });

        if (!response?.CategoryEvents) {
            console.warn("No CategoryEvents in response; continuing...");
        } else {
            for (const ev of response.CategoryEvents) {
                const id = normaliseRoomName(ev.Name);
                if (!roomMap.has(id)) roomMap.set(id, ev.Identity);
            }
            console.log(`Added ${response.CategoryEvents.length} CategoryEvents (map size now ${roomMap.size})`);
        }

        await deselectBatch(page, batchIndices, clickDelayMs);

        processed = end;
    }

    console.log("\n🏁 Finished collection.");
    console.table([...roomMap.entries()].map(([abbr, identity]) => ({ abbr, identity })));

    await browser.close();
    return roomMap;
}
