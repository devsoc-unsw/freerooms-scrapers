import { chromium } from "playwright";

const fetchXlsx = async (url: string, filename: string) => {
  // Setup page and go to url
  const browser = await chromium.launch({ headless: false });
  const zoomedBrowser = await browser.newContext({
    viewport: { width: 1920, height: 1080 },
  });
  const page = await zoomedBrowser.newPage();
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
  await zoomedBrowser.close();
  await browser.close();
};

export default fetchXlsx;
