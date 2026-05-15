/**
 * FFmpeg Video Processing Helpers for Node.js
 * ============================================
 * Uses child_process to invoke ffmpeg/ffprobe CLI tools.
 * Requires: ffmpeg and ffprobe installed and available in PATH.
 *
 * Optional dependency: fluent-ffmpeg (npm install fluent-ffmpeg)
 * This file uses the raw CLI approach for zero-dependency operation.
 */

'use strict';

const { execFile, spawn } = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { promisify } = require('util');

const execFileAsync = promisify(execFile);

// ---------------------------------------------------------------------------
// Probe / Inspect
// ---------------------------------------------------------------------------

/**
 * Probe a media file and return parsed JSON from ffprobe.
 *
 * @param {string} inputPath - Path to the media file.
 * @returns {Promise<object>} Parsed ffprobe output with format and streams.
 */
async function probeMedia(inputPath) {
  const { stdout } = await execFileAsync('ffprobe', [
    '-v', 'quiet',
    '-print_format', 'json',
    '-show_format',
    '-show_streams',
    inputPath,
  ]);
  return JSON.parse(stdout);
}

/**
 * Get the duration of a media file in seconds.
 *
 * @param {string} inputPath
 * @returns {Promise<number>} Duration in seconds.
 */
async function getDuration(inputPath) {
  const { stdout } = await execFileAsync('ffprobe', [
    '-v', 'quiet',
    '-show_entries', 'format=duration',
    '-of', 'default=noprint_wrappers=1:nokey=1',
    inputPath,
  ]);
  return parseFloat(stdout.trim());
}

/**
 * Get metadata for the first video stream.
 *
 * @param {string} inputPath
 * @returns {Promise<object|null>}
 */
async function getVideoInfo(inputPath) {
  const info = await probeMedia(inputPath);
  return (info.streams || []).find(s => s.codec_type === 'video') || null;
}

/**
 * Get metadata for the first audio stream.
 *
 * @param {string} inputPath
 * @returns {Promise<object|null>}
 */
async function getAudioInfo(inputPath) {
  const info = await probeMedia(inputPath);
  return (info.streams || []).find(s => s.codec_type === 'audio') || null;
}

// ---------------------------------------------------------------------------
// Core FFmpeg runner
// ---------------------------------------------------------------------------

/**
 * Run an FFmpeg command and return a Promise that resolves when complete.
 * Streams stderr to process.stderr for live progress.
 *
 * @param {string[]} args - FFmpeg arguments (excluding the 'ffmpeg' binary name).
 * @param {object} [options]
 * @param {boolean} [options.quiet=false] - Suppress FFmpeg stderr output.
 * @returns {Promise<void>}
 */
function runFfmpeg(args, { quiet = false } = {}) {
  return new Promise((resolve, reject) => {
    const proc = spawn('ffmpeg', args, { stdio: quiet ? 'pipe' : ['pipe', 'pipe', 'inherit'] });

    proc.on('close', code => {
      if (code === 0) {
        resolve();
      } else {
        reject(new Error(`FFmpeg exited with code ${code}. Args: ${args.join(' ')}`));
      }
    });

    proc.on('error', reject);
  });
}

// ---------------------------------------------------------------------------
// Transcoding
// ---------------------------------------------------------------------------

/**
 * Transcode a video to H.264 + AAC in an MP4 container.
 *
 * @param {string} inputPath - Source video.
 * @param {string} outputPath - Output MP4 file.
 * @param {object} [options]
 * @param {number} [options.crf=23] - Constant Rate Factor quality (18–28 typical).
 * @param {string} [options.preset='medium'] - Encoding speed preset.
 * @param {string} [options.audioBitrate='192k'] - AAC audio bit rate.
 * @returns {Promise<void>}
 */
async function transcodeToH264(inputPath, outputPath, {
  crf = 23,
  preset = 'medium',
  audioBitrate = '192k',
} = {}) {
  await runFfmpeg([
    '-y',
    '-i', inputPath,
    '-c:v', 'libx264',
    '-crf', String(crf),
    '-preset', preset,
    '-pix_fmt', 'yuv420p',
    '-movflags', '+faststart',
    '-c:a', 'aac',
    '-b:a', audioBitrate,
    '-ar', '48000',
    outputPath,
  ]);
}

/**
 * Transcode a video to H.265/HEVC + AAC in an MP4 container.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @param {object} [options]
 * @param {number} [options.crf=28] - CRF quality for HEVC (24–32 typical).
 * @param {string} [options.preset='slow']
 * @returns {Promise<void>}
 */
async function transcodeToHEVC(inputPath, outputPath, { crf = 28, preset = 'slow' } = {}) {
  await runFfmpeg([
    '-y',
    '-i', inputPath,
    '-c:v', 'libx265',
    '-crf', String(crf),
    '-preset', preset,
    '-pix_fmt', 'yuv420p',
    '-tag:v', 'hvc1',
    '-movflags', '+faststart',
    '-c:a', 'aac',
    '-b:a', '192k',
    outputPath,
  ]);
}

/**
 * Remux (stream copy) to a different container without re-encoding.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @returns {Promise<void>}
 */
async function remux(inputPath, outputPath) {
  await runFfmpeg(['-y', '-i', inputPath, '-c', 'copy', outputPath]);
}

// ---------------------------------------------------------------------------
// Trimming
// ---------------------------------------------------------------------------

/**
 * Trim a media file between two time positions.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @param {object} options
 * @param {number} options.start - Start time in seconds.
 * @param {number} [options.end] - End time in seconds.
 * @param {number} [options.duration] - Duration in seconds from start.
 * @param {boolean} [options.reencode=false] - Re-encode for frame-accurate trim.
 * @returns {Promise<void>}
 */
async function trim(inputPath, outputPath, { start, end, duration, reencode = false }) {
  if (end !== undefined && duration !== undefined) {
    throw new Error('Provide either end or duration, not both.');
  }

  const args = ['-y', '-ss', String(start), '-i', inputPath];

  if (end !== undefined) {
    args.push('-to', String(end - start));
  } else if (duration !== undefined) {
    args.push('-t', String(duration));
  }

  if (reencode) {
    args.push('-c:v', 'libx264', '-crf', '23', '-c:a', 'aac');
  } else {
    args.push('-c', 'copy');
  }

  args.push(outputPath);
  await runFfmpeg(args);
}

// ---------------------------------------------------------------------------
// Scaling
// ---------------------------------------------------------------------------

/**
 * Resize a video to the specified dimensions.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @param {object} options
 * @param {number} options.width - Target width (use -2 for auto).
 * @param {number} [options.height=-2] - Target height (-2 = maintain aspect ratio).
 * @returns {Promise<void>}
 */
async function resize(inputPath, outputPath, { width, height = -2 }) {
  await runFfmpeg([
    '-y',
    '-i', inputPath,
    '-vf', `scale=${width}:${height}:flags=lanczos`,
    '-c:v', 'libx264',
    '-crf', '23',
    '-c:a', 'copy',
    outputPath,
  ]);
}

// ---------------------------------------------------------------------------
// Thumbnails
// ---------------------------------------------------------------------------

/**
 * Extract a single thumbnail frame from a video.
 *
 * @param {string} inputPath
 * @param {string} outputPath - Output JPEG file path.
 * @param {object} [options]
 * @param {number} [options.timestamp=5] - Position in seconds.
 * @param {number} [options.width=1280] - Thumbnail width.
 * @returns {Promise<void>}
 */
async function extractThumbnail(inputPath, outputPath, { timestamp = 5, width = 1280 } = {}) {
  await runFfmpeg([
    '-y',
    '-ss', String(timestamp),
    '-i', inputPath,
    '-vf', `scale=${width}:-2`,
    '-frames:v', '1',
    '-q:v', '2',
    outputPath,
  ], { quiet: true });
}

/**
 * Extract thumbnail frames at a regular interval.
 *
 * @param {string} inputPath
 * @param {string} outputDir - Directory to write thumbnails.
 * @param {object} [options]
 * @param {number} [options.interval=10] - Interval in seconds.
 * @param {number} [options.width=320] - Thumbnail width.
 * @returns {Promise<string[]>} Sorted list of generated thumbnail paths.
 */
async function extractThumbnailsAtInterval(inputPath, outputDir, { interval = 10, width = 320 } = {}) {
  fs.mkdirSync(outputDir, { recursive: true });
  const pattern = path.join(outputDir, 'thumb_%04d.jpg');

  await runFfmpeg([
    '-y',
    '-i', inputPath,
    '-vf', `fps=1/${interval},scale=${width}:-2`,
    '-q:v', '3',
    pattern,
  ], { quiet: true });

  return fs.readdirSync(outputDir)
    .filter(f => f.startsWith('thumb_') && f.endsWith('.jpg'))
    .sort()
    .map(f => path.join(outputDir, f));
}

// ---------------------------------------------------------------------------
// Audio operations
// ---------------------------------------------------------------------------

/**
 * Extract audio from a video file.
 *
 * @param {string} inputPath
 * @param {string} outputPath - Audio output (format by extension, e.g. .wav, .mp3).
 * @returns {Promise<void>}
 */
async function extractAudio(inputPath, outputPath) {
  await runFfmpeg(['-y', '-i', inputPath, '-vn', '-c:a', 'copy', outputPath]);
}

/**
 * Extract audio as uncompressed PCM WAV.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @param {number} [sampleRate=48000]
 * @returns {Promise<void>}
 */
async function extractAudioAsWav(inputPath, outputPath, sampleRate = 48000) {
  await runFfmpeg([
    '-y', '-i', inputPath,
    '-vn', '-c:a', 'pcm_s16le',
    '-ar', String(sampleRate),
    outputPath,
  ]);
}

/**
 * Remove all audio from a video file.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @returns {Promise<void>}
 */
async function removeAudio(inputPath, outputPath) {
  await runFfmpeg(['-y', '-i', inputPath, '-c:v', 'copy', '-an', outputPath]);
}

/**
 * Replace the audio track in a video with a new audio file.
 *
 * @param {string} videoPath
 * @param {string} audioPath
 * @param {string} outputPath
 * @returns {Promise<void>}
 */
async function replaceAudio(videoPath, audioPath, outputPath) {
  await runFfmpeg([
    '-y',
    '-i', videoPath,
    '-i', audioPath,
    '-map', '0:v',
    '-map', '1:a',
    '-c:v', 'copy',
    '-c:a', 'aac', '-b:a', '192k',
    '-shortest',
    outputPath,
  ]);
}

// ---------------------------------------------------------------------------
// Watermarking
// ---------------------------------------------------------------------------

/**
 * Burn a text watermark into a video.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @param {object} options
 * @param {string} options.text - Watermark text.
 * @param {number} [options.fontSize=28] - Font size.
 * @param {number} [options.opacity=0.6] - Text opacity (0–1).
 * @param {string} [options.position='bottom-right'] - Position on frame.
 * @returns {Promise<void>}
 */
async function addTextWatermark(inputPath, outputPath, {
  text,
  fontSize = 28,
  opacity = 0.6,
  position = 'bottom-right',
}) {
  const positions = {
    'top-left':     'x=20:y=20',
    'top-right':    'x=W-tw-20:y=20',
    'bottom-left':  'x=20:y=H-th-20',
    'bottom-right': 'x=W-tw-20:y=H-th-20',
  };
  const xy = positions[position] || positions['bottom-right'];

  const escapedText = text.replace(/'/g, "\\'").replace(/:/g, '\\:');
  const vf = `drawtext=text='${escapedText}':fontcolor=white@${opacity}:fontsize=${fontSize}:${xy}`;

  await runFfmpeg([
    '-y', '-i', inputPath,
    '-vf', vf,
    '-c:v', 'libx264', '-crf', '23',
    '-c:a', 'copy',
    outputPath,
  ]);
}

// ---------------------------------------------------------------------------
// Concatenation
// ---------------------------------------------------------------------------

/**
 * Concatenate multiple video files using the concat demuxer (stream copy).
 * All inputs must have the same codec, resolution, and frame rate.
 *
 * @param {string[]} inputPaths - Ordered list of video files.
 * @param {string} outputPath
 * @returns {Promise<void>}
 */
async function concatenateVideos(inputPaths, outputPath) {
  const listPath = path.join(os.tmpdir(), `ffmpeg_concat_${Date.now()}.txt`);
  const content = inputPaths
    .map(p => `file '${path.resolve(p)}'`)
    .join('\n');
  fs.writeFileSync(listPath, content);

  try {
    await runFfmpeg([
      '-y',
      '-f', 'concat',
      '-safe', '0',
      '-i', listPath,
      '-c', 'copy',
      outputPath,
    ]);
  } finally {
    fs.unlinkSync(listPath);
  }
}

// ---------------------------------------------------------------------------
// GIF Creation
// ---------------------------------------------------------------------------

/**
 * Create an optimized GIF from a video clip.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @param {object} [options]
 * @param {number} [options.start=0] - Start time in seconds.
 * @param {number} [options.duration=5] - Duration in seconds.
 * @param {number} [options.fps=15] - GIF frame rate.
 * @param {number} [options.width=480] - GIF width.
 * @returns {Promise<void>}
 */
async function createGif(inputPath, outputPath, {
  start = 0,
  duration = 5,
  fps = 15,
  width = 480,
} = {}) {
  const filterGraph = [
    `[0:v] fps=${fps},scale=${width}:-1,split [a][b]`,
    '[a] palettegen [p]',
    '[b][p] paletteuse',
  ].join(';');

  await runFfmpeg([
    '-y',
    '-ss', String(start),
    '-t', String(duration),
    '-i', inputPath,
    '-filter_complex', filterGraph,
    outputPath,
  ]);
}

// ---------------------------------------------------------------------------
// Progress tracking
// ---------------------------------------------------------------------------

/**
 * Transcode with real-time progress reporting.
 *
 * @param {string} inputPath
 * @param {string} outputPath
 * @param {string[]} ffmpegArgs - Additional FFmpeg arguments (codec, filters, etc.).
 * @param {Function} onProgress - Callback called with progress object:
 *   { frame, fps, bitrate, totalSize, outTime, speed, percent }
 * @param {number} [totalDurationSec] - Total duration for percent calculation.
 * @returns {Promise<void>}
 */
function transcodeWithProgress(inputPath, outputPath, ffmpegArgs, onProgress, totalDurationSec) {
  return new Promise((resolve, reject) => {
    const args = [
      '-y',
      '-i', inputPath,
      ...ffmpegArgs,
      '-progress', 'pipe:1',
      '-nostats',
      outputPath,
    ];

    const proc = spawn('ffmpeg', args, { stdio: ['pipe', 'pipe', 'pipe'] });
    let progressData = {};

    proc.stdout.on('data', (data) => {
      const lines = data.toString().split('\n');
      for (const line of lines) {
        const [key, value] = line.split('=');
        if (key && value) {
          progressData[key.trim()] = value.trim();
        }
        if (key && key.trim() === 'progress') {
          // Parse out_time to calculate percent
          const outTime = progressData.out_time || '00:00:00.000';
          const parts = outTime.split(':');
          const currentSec = (parseFloat(parts[0]) * 3600)
            + (parseFloat(parts[1]) * 60)
            + parseFloat(parts[2]);

          const percent = totalDurationSec
            ? Math.min(100, (currentSec / totalDurationSec) * 100)
            : null;

          onProgress({
            frame: parseInt(progressData.frame, 10) || 0,
            fps: parseFloat(progressData.fps) || 0,
            bitrate: progressData.bitrate || '0kbits/s',
            totalSize: parseInt(progressData.total_size, 10) || 0,
            outTime: outTime,
            speed: progressData.speed || '0x',
            percent,
          });

          progressData = {};
        }
      }
    });

    proc.on('close', code => {
      if (code === 0) resolve();
      else reject(new Error(`FFmpeg exited with code ${code}`));
    });

    proc.on('error', reject);
  });
}

// ---------------------------------------------------------------------------
// Exports
// ---------------------------------------------------------------------------

module.exports = {
  // Probe
  probeMedia,
  getDuration,
  getVideoInfo,
  getAudioInfo,

  // Transcode
  transcodeToH264,
  transcodeToHEVC,
  remux,

  // Edit
  trim,
  resize,

  // Thumbnails
  extractThumbnail,
  extractThumbnailsAtInterval,

  // Audio
  extractAudio,
  extractAudioAsWav,
  removeAudio,
  replaceAudio,

  // Effects
  addTextWatermark,
  createGif,

  // Concatenation
  concatenateVideos,

  // Advanced
  runFfmpeg,
  transcodeWithProgress,
};
