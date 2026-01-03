import { Browser, chromium, Page } from "playwright";

const fetchXlsx = async (url: string, filename: string) => {
  // Setup page and go to url
  const browser: Browser = await chromium.launch({ headless: false });
  const page: Page = await browser.newPage();
  await page.goto(url);

  // Get click excel button
  const downloadPromise = page.waitForEvent("download");
  await page.waitForSelector(".e-excel-export button");
  await page.waitForSelector(".e-appointment-details div");
  await page.click(".e-excel-export button");
  const download = await downloadPromise;

  // Save file
  await download.saveAs(`${filename}.xlsx`);

  // Close browser
  await browser.close();
};

export default fetchXlsx;

