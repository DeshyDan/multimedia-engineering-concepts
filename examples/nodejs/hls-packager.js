/**
 * FFmpeg HLS Packager for Node.js
 * ================================
 * Generates a multi-bitrate HLS package from a single source video.
 *
 * Usage:
 *   node hls-packager.js input.mp4 ./output_hls
 *   node hls-packager.js input.mp4 ./output_hls --segment-duration 4
 *
 * Requirements:
 *   - ffmpeg with libx264 support
 *   - Node.js 16+
 */

'use strict';

const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');

// ---------------------------------------------------------------------------
// Default ABR ladder configuration
// ---------------------------------------------------------------------------

/**
 * @typedef {object} Rendition
 * @property {string} name - Rendition name (e.g., "1080p")
 * @property {number} width - Video width
 * @property {number} height - Video height
 * @property {number} videoBitrateKbps - Video bit rate in kbps
 * @property {number} audioBitrateKbps - Audio bit rate in kbps
 */

/** @type {Rendition[]} */
const DEFAULT_RENDITIONS = [
  { name: '1080p', width: 1920, height: 1080, videoBitrateKbps: 4500, audioBitrateKbps: 192 },
  { name: '720p',  width: 1280, height: 720,  videoBitrateKbps: 2500, audioBitrateKbps: 128 },
  { name: '480p',  width: 854,  height: 480,  videoBitrateKbps: 1000, audioBitrateKbps: 96  },
  { name: '360p',  width: 640,  height: 360,  videoBitrateKbps: 500,  audioBitrateKbps: 64  },
];

// ---------------------------------------------------------------------------
// Command builder
// ---------------------------------------------------------------------------

/**
 * Build the FFmpeg argument list for multi-bitrate HLS packaging.
 *
 * @param {object} params
 * @param {string} params.inputPath - Source video file.
 * @param {string} params.outputDir - Directory for HLS output.
 * @param {Rendition[]} params.renditions - Array of rendition configs.
 * @param {number} [params.segmentDuration=6] - HLS segment duration in seconds.
 * @param {number} [params.keyframeInterval=60] - Keyframe interval (frames).
 * @param {boolean} [params.useFmp4=true] - Use fMP4 segments (vs MPEG-TS).
 * @returns {string[]} FFmpeg arguments.
 */
function buildHlsArgs({
  inputPath,
  outputDir,
  renditions,
  segmentDuration = 6,
  keyframeInterval = 60,
  useFmp4 = true,
}) {
  const n = renditions.length;

  // Video split + scale filter
  const splitPads = renditions.map((_, i) => `[v${i}]`).join('');
  const filterParts = [`[0:v]split=${n}${splitPads}`];
  renditions.forEach((r, i) => {
    filterParts.push(`[v${i}]scale=${r.width}:${r.height}[vout${i}]`);
  });
  const filterComplex = filterParts.join('; ');

  const args = ['-y', '-i', inputPath, '-filter_complex', filterComplex];

  // Per-rendition video encoding options
  renditions.forEach((r, i) => {
    const bv = `${r.videoBitrateKbps}k`;
    const maxrate = `${Math.round(r.videoBitrateKbps * 1.07)}k`;
    const bufsize = `${r.videoBitrateKbps * 2}k`;
    args.push(
      '-map', `[vout${i}]`,
      `-c:v:${i}`, 'libx264',
      `-b:v:${i}`, bv,
      `-maxrate:v:${i}`, maxrate,
      `-bufsize:v:${i}`, bufsize,
      `-preset:v:${i}`, 'fast',
    );
  });

  // Per-rendition audio encoding options
  renditions.forEach((r, i) => {
    args.push(
      '-map', '0:a',
      `-c:a:${i}`, 'aac',
      `-b:a:${i}`, `${r.audioBitrateKbps}k`,
      `-ar:${i}`, '48000',
    );
  });

  // Fixed keyframe interval (critical for HLS segment alignment)
  args.push(
    '-x264opts', `keyint=${keyframeInterval}:min-keyint=${keyframeInterval}:no-scenecut`,
    '-g', String(keyframeInterval),
    '-keyint_min', String(keyframeInterval),
    '-sc_threshold', '0',
  );

  // HLS output options
  const segExt = useFmp4 ? 'mp4' : 'ts';
  const masterPl = path.join(outputDir, 'master.m3u8');
  const varStreamMap = renditions.map((_, i) => `v:${i},a:${i}`).join(' ');
  const segmentFilename = path.join(outputDir, 'stream_%v', `seg_%03d.${segExt}`);
  const variantPlaylist = path.join(outputDir, 'stream_%v', 'playlist.m3u8');

  args.push(
    '-f', 'hls',
    '-hls_time', String(segmentDuration),
    '-hls_playlist_type', 'vod',
    '-hls_flags', 'independent_segments',
  );

  if (useFmp4) {
    args.push('-hls_segment_type', 'fmp4');
  }

  args.push(
    '-master_pl_name', masterPl,
    '-var_stream_map', varStreamMap,
    '-hls_segment_filename', segmentFilename,
    variantPlaylist,
  );

  return args;
}

// ---------------------------------------------------------------------------
// HLS packager
// ---------------------------------------------------------------------------

/**
 * Package a video as multi-bitrate HLS.
 *
 * @param {object} params
 * @param {string} params.inputPath - Source video.
 * @param {string} params.outputDir - Output directory.
 * @param {Rendition[]} [params.renditions] - Custom renditions (default ladder if omitted).
 * @param {number} [params.segmentDuration=6] - HLS segment duration.
 * @param {Function} [params.onLog] - Called with each FFmpeg stderr line.
 * @returns {Promise<void>}
 */
function packageHls({
  inputPath,
  outputDir,
  renditions = DEFAULT_RENDITIONS,
  segmentDuration = 6,
  onLog = null,
}) {
  // Create rendition subdirectories
  renditions.forEach((_, i) => {
    fs.mkdirSync(path.join(outputDir, `stream_${i}`), { recursive: true });
  });

  const args = buildHlsArgs({ inputPath, outputDir, renditions, segmentDuration });

  console.log(`Packaging HLS with ${renditions.length} renditions...`);
  console.log(`Renditions: ${renditions.map(r => r.name).join(', ')}`);

  return new Promise((resolve, reject) => {
    const proc = spawn('ffmpeg', args, { stdio: ['pipe', 'pipe', 'pipe'] });

    proc.stderr.on('data', (data) => {
      const line = data.toString();
      if (onLog) {
        onLog(line);
      } else {
        process.stderr.write(line);
      }
    });

    proc.on('close', (code) => {
      if (code === 0) {
        console.log(`\nHLS output: ${outputDir}`);
        console.log(`Master playlist: ${path.join(outputDir, 'master.m3u8')}`);
        resolve();
      } else {
        reject(new Error(`FFmpeg exited with code ${code}`));
      }
    });

    proc.on('error', reject);
  });
}

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------

if (require.main === module) {
  const args = process.argv.slice(2);

  const inputPath = args[0];
  const outputDir = args[1];

  if (!inputPath || !outputDir) {
    console.error('Usage: node hls-packager.js <input> <output_dir> [--segment-duration N]');
    process.exit(1);
  }

  if (!fs.existsSync(inputPath)) {
    console.error(`Input file not found: ${inputPath}`);
    process.exit(1);
  }

  const segIdx = args.indexOf('--segment-duration');
  const segmentDuration = segIdx >= 0 ? parseInt(args[segIdx + 1], 10) : 6;

  packageHls({ inputPath, outputDir, segmentDuration })
    .then(() => console.log('Done.'))
    .catch((err) => {
      console.error('Error:', err.message);
      process.exit(1);
    });
}

module.exports = { packageHls, buildHlsArgs, DEFAULT_RENDITIONS };
