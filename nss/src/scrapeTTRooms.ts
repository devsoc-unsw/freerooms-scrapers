
import fs from "fs";
import { Browser, chromium, Page } from "playwright";

const TT_URL = "https://publish.sit1.unsw.edu.au/timetables?date=2026-02-16&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&selections=1e042cb1-547d-41d4-ae93-a1f2c3d34538__";
const BATCH_SIZE = 20;

const fetchXlsx = async (url: string, filename: string) => {
    // Setup page and go to url
    const browser: Browser = await chromium.launch({ headless: false });
    const page: Page = await browser.newPage();
    
    try {
        await page.goto(url);
        // Get click excel button
        const downloadPromise = page.waitForEvent("download");
        await page.waitForSelector(".e-excel-export button");
        await page.waitForSelector(".e-appointment-details div");
        await page.click(".e-excel-export button");
        const download = await downloadPromise;
      
        await download.saveAs(`${filename}.xlsx`);
    } catch (error) {
        console.error(`Error fetching ${url}: ${error}`);
    } finally {
        await browser.close();
    }
  };

const scrapeTTRooms = async () => {
    const sessionIdentitiesRaw = fs.readFileSync("../sessionIdentities.json", "utf8");
    const sessionIdentities = JSON.parse(sessionIdentitiesRaw);

    // Process batches of 20 rooms
    for (let i = 0; i < 60; i += BATCH_SIZE) {
        const batch = sessionIdentities.slice(i, i + BATCH_SIZE);
        const url = TT_URL + batch.map((s: { Room: string; Identity: string }) => s.Identity).join("_");
        console.log(`Processing batch ${i} to ${i + BATCH_SIZE}...`);
        
        try {
            setTimeout(async () => {
                await fetchXlsx(url, `./${i}.xlsx`);
            }, 3000);
        } catch (error) {
            console.log(`Continuing with next batch...`);
        }
    }
}

scrapeTTRooms();