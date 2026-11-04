import { ElevenLabsClient } from '@elevenlabs/elevenlabs-js';
import dotenv from 'dotenv';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
dotenv.config();

const VOICE_ID = '0L5MHtpqE3W34LBU9m2a';
const MODEL_ID = 'eleven_multilingual_v2';
const OUTPUT_FORMAT = 'mp3_44100_128';
const CONCURRENCY = 5;

const FILES = [
  // { src: 'lines_detection.txt', outDir: 'output/detection' },
  // { src: 'lines_scheduled.txt', outDir: 'output/scheduled' },
  // { src: 'lines_congrats_timmy.txt', outDir: 'output/congrats/timmy' },
  // { src: 'lines_congrats_luke.txt', outDir: 'output/congrats/luke' },
  { src: 'lines_welcome.txt', outDir: 'output/welcome' },
];

const elevenlabs = new ElevenLabsClient({
  apiKey: process.env.ELEVENLABS_API_KEY,
});

async function generateOne(text, outPath) {
  const audio = await elevenlabs.textToSpeech.convert(VOICE_ID, {
    text,
    modelId: MODEL_ID,
    outputFormat: OUTPUT_FORMAT,
  });

  const chunks = [];
  for await (const chunk of audio) {
    chunks.push(chunk);
  }
  await writeFile(outPath, Buffer.concat(chunks));
}

async function processInBatches(tasks, concurrency) {
  for (let i = 0; i < tasks.length; i += concurrency) {
    const batch = tasks.slice(i, i + concurrency);
    await Promise.all(batch.map((fn) => fn()));
  }
}

const allTasks = [];

for (const { src, outDir } of FILES) {
  await mkdir(outDir, { recursive: true });

  const content = await readFile(src, 'utf-8');
  const lines = content.split('\n').filter((l) => l.trim());

  for (let i = 0; i < lines.length; i++) {
    const idx = i + 1;
    const outPath = `${outDir}/${idx}.mp3`;
    allTasks.push(() => {
      console.log(`[${outDir}] Generating ${idx}/${lines.length}...`);
      return generateOne(lines[i], outPath);
    });
  }
}

console.log(`Total: ${allTasks.length} files to generate (concurrency: ${CONCURRENCY})`);
await processInBatches(allTasks, CONCURRENCY);
console.log('Done!');
