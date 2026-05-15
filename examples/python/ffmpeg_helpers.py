"""
FFmpeg Video Processing Examples in Python
==========================================
Uses subprocess to call ffmpeg/ffprobe CLI tools.
Requires: ffmpeg and ffprobe installed and available in PATH.

Install ffmpeg-python wrapper (optional):
    pip install ffmpeg-python
"""

import json
import os
import subprocess
from pathlib import Path


# ---------------------------------------------------------------------------
# Probe / Inspect
# ---------------------------------------------------------------------------

def probe_media(input_path: str) -> dict:
    """
    Return a dict with format and stream information for a media file.

    Args:
        input_path: Path to the media file.

    Returns:
        Parsed JSON output from ffprobe.
    """
    result = subprocess.run(
        [
            "ffprobe",
            "-v", "quiet",
            "-print_format", "json",
            "-show_format",
            "-show_streams",
            input_path,
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    return json.loads(result.stdout)


def get_duration(input_path: str) -> float:
    """Return the duration of a media file in seconds."""
    result = subprocess.run(
        [
            "ffprobe",
            "-v", "quiet",
            "-show_entries", "format=duration",
            "-of", "default=noprint_wrappers=1:nokey=1",
            input_path,
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    return float(result.stdout.strip())


def get_video_info(input_path: str) -> dict | None:
    """Return metadata for the first video stream."""
    info = probe_media(input_path)
    for stream in info.get("streams", []):
        if stream.get("codec_type") == "video":
            return stream
    return None


def get_audio_info(input_path: str) -> dict | None:
    """Return metadata for the first audio stream."""
    info = probe_media(input_path)
    for stream in info.get("streams", []):
        if stream.get("codec_type") == "audio":
            return stream
    return None


# ---------------------------------------------------------------------------
# Transcoding
# ---------------------------------------------------------------------------

def transcode_to_h264(
    input_path: str,
    output_path: str,
    crf: int = 23,
    preset: str = "medium",
    audio_bitrate: str = "192k",
) -> None:
    """
    Transcode a video to H.264 (libx264) + AAC in an MP4 container.

    Args:
        input_path: Path to source video.
        output_path: Path to write the transcoded MP4.
        crf: Constant Rate Factor quality (0–51; lower = better, 18–28 typical).
        preset: Encoding speed/quality preset (ultrafast … veryslow).
        audio_bitrate: AAC audio bit rate (e.g. "192k").
    """
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", input_path,
            "-c:v", "libx264",
            "-crf", str(crf),
            "-preset", preset,
            "-pix_fmt", "yuv420p",
            "-movflags", "+faststart",
            "-c:a", "aac",
            "-b:a", audio_bitrate,
            "-ar", "48000",
            output_path,
        ],
        check=True,
    )


def transcode_to_hevc(
    input_path: str,
    output_path: str,
    crf: int = 28,
    preset: str = "slow",
) -> None:
    """
    Transcode a video to H.265/HEVC (libx265) + AAC in an MP4 container.

    Args:
        input_path: Path to source video.
        output_path: Path to write the output.
        crf: Constant Rate Factor (24–32 typical for HEVC).
        preset: Encoding preset.
    """
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", input_path,
            "-c:v", "libx265",
            "-crf", str(crf),
            "-preset", preset,
            "-pix_fmt", "yuv420p",
            "-tag:v", "hvc1",         # Required for Apple device compatibility
            "-movflags", "+faststart",
            "-c:a", "aac",
            "-b:a", "192k",
            output_path,
        ],
        check=True,
    )


def remux(input_path: str, output_path: str) -> None:
    """
    Remux (copy all streams without re-encoding) to a new container.

    Args:
        input_path: Source file.
        output_path: Destination file (container determined by extension).
    """
    subprocess.run(
        ["ffmpeg", "-y", "-i", input_path, "-c", "copy", output_path],
        check=True,
    )


# ---------------------------------------------------------------------------
# Cutting & Trimming
# ---------------------------------------------------------------------------

def trim(
    input_path: str,
    output_path: str,
    start: float,
    end: float | None = None,
    duration: float | None = None,
    reencode: bool = False,
) -> None:
    """
    Trim a media file between two time points.

    Args:
        input_path: Source file.
        output_path: Destination file.
        start: Start time in seconds.
        end: End time in seconds (mutually exclusive with duration).
        duration: Duration in seconds from start (mutually exclusive with end).
        reencode: If False, stream copy (fast but may be slightly inaccurate
                  at non-keyframe positions). If True, re-encode for accuracy.
    """
    if end is not None and duration is not None:
        raise ValueError("Provide either end or duration, not both.")

    cmd = ["ffmpeg", "-y", "-ss", str(start), "-i", input_path]

    if end is not None:
        # When using input seeking, -to is relative to -ss
        cmd += ["-to", str(end - start)]
    elif duration is not None:
        cmd += ["-t", str(duration)]

    if reencode:
        cmd += ["-c:v", "libx264", "-crf", "23", "-c:a", "aac"]
    else:
        cmd += ["-c", "copy"]

    cmd.append(output_path)
    subprocess.run(cmd, check=True)


# ---------------------------------------------------------------------------
# Resizing & Scaling
# ---------------------------------------------------------------------------

def resize(input_path: str, output_path: str, width: int, height: int = -2) -> None:
    """
    Resize video to the given dimensions.

    Args:
        input_path: Source video.
        output_path: Output video.
        width: Target width in pixels.
        height: Target height in pixels. Use -2 to maintain aspect ratio
                (automatically calculated to be divisible by 2).
    """
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", input_path,
            "-vf", f"scale={width}:{height}:flags=lanczos",
            "-c:v", "libx264", "-crf", "23",
            "-c:a", "copy",
            output_path,
        ],
        check=True,
    )


def scale_to_720p(input_path: str, output_path: str) -> None:
    """Scale video to 1280×720 (HD) maintaining aspect ratio."""
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", input_path,
            "-vf", "scale=1280:720:force_original_aspect_ratio=decrease,"
                   "pad=1280:720:(ow-iw)/2:(oh-ih)/2",
            "-c:v", "libx264", "-crf", "23",
            "-c:a", "copy",
            output_path,
        ],
        check=True,
    )


# ---------------------------------------------------------------------------
# Thumbnails & Screenshots
# ---------------------------------------------------------------------------

def extract_thumbnail(
    input_path: str,
    output_path: str,
    timestamp: float = 5.0,
    width: int = 1280,
) -> None:
    """
    Extract a single frame from a video as a JPEG thumbnail.

    Args:
        input_path: Source video.
        output_path: Output JPEG file path.
        timestamp: Position in seconds from which to extract the frame.
        width: Width of the thumbnail (height auto-calculated).
    """
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-ss", str(timestamp),
            "-i", input_path,
            "-vf", f"scale={width}:-2",
            "-frames:v", "1",
            "-q:v", "2",
            output_path,
        ],
        check=True,
    )


def extract_thumbnails_at_interval(
    input_path: str,
    output_dir: str,
    interval: float = 10.0,
    width: int = 320,
) -> list[str]:
    """
    Extract thumbnails at regular intervals from a video.

    Args:
        input_path: Source video.
        output_dir: Directory to store thumbnail images.
        interval: Interval in seconds between thumbnails.
        width: Thumbnail width.

    Returns:
        List of generated thumbnail file paths.
    """
    os.makedirs(output_dir, exist_ok=True)
    pattern = os.path.join(output_dir, "thumb_%04d.jpg")

    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", input_path,
            "-vf", f"fps=1/{interval},scale={width}:-2",
            "-q:v", "3",
            pattern,
        ],
        check=True,
    )

    return sorted(Path(output_dir).glob("thumb_*.jpg"))


# ---------------------------------------------------------------------------
# Audio Operations
# ---------------------------------------------------------------------------

def extract_audio(input_path: str, output_path: str) -> None:
    """
    Extract audio from a video file. Output format determined by extension.

    Args:
        input_path: Source video file.
        output_path: Audio output file (e.g., output.wav, output.mp3).
    """
    subprocess.run(
        ["ffmpeg", "-y", "-i", input_path, "-vn", "-c:a", "copy", output_path],
        check=True,
    )


def extract_audio_as_wav(input_path: str, output_path: str, sample_rate: int = 48000) -> None:
    """
    Extract and convert audio to uncompressed PCM WAV.

    Args:
        input_path: Source media file.
        output_path: WAV output path.
        sample_rate: Audio sample rate in Hz (default 48000).
    """
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", input_path,
            "-vn",
            "-c:a", "pcm_s16le",
            "-ar", str(sample_rate),
            output_path,
        ],
        check=True,
    )


def normalize_loudness(
    input_path: str,
    output_path: str,
    target_lufs: float = -14.0,
    true_peak: float = -1.5,
    lra: float = 11.0,
) -> None:
    """
    Normalize audio loudness to EBU R128 standard using a two-pass approach.

    Args:
        input_path: Source audio or video.
        output_path: Output file with normalized audio.
        target_lufs: Target integrated loudness (default -14 LUFS for streaming).
        true_peak: Maximum true peak level in dBTP.
        lra: Loudness range target (LU).
    """
    # Pass 1: Measure loudness statistics
    result = subprocess.run(
        [
            "ffmpeg",
            "-i", input_path,
            "-af", f"loudnorm=I={target_lufs}:LRA={lra}:TP={true_peak}:print_format=json",
            "-f", "null", "-",
        ],
        capture_output=True,
        text=True,
    )

    # Parse JSON from stderr (loudnorm prints to stderr)
    output = result.stderr
    json_start = output.rfind("{")
    json_end = output.rfind("}") + 1
    measured = json.loads(output[json_start:json_end])

    # Pass 2: Apply normalization using measured values
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", input_path,
            "-af",
            f"loudnorm=I={target_lufs}:LRA={lra}:TP={true_peak}:"
            f"measured_I={measured['input_i']}:"
            f"measured_LRA={measured['input_lra']}:"
            f"measured_TP={measured['input_tp']}:"
            f"measured_thresh={measured['input_thresh']}:"
            f"offset={measured['target_offset']}:"
            f"linear=true",
            "-c:v", "copy",
            output_path,
        ],
        check=True,
    )


def remove_audio(input_path: str, output_path: str) -> None:
    """Remove all audio streams from a video file."""
    subprocess.run(
        ["ffmpeg", "-y", "-i", input_path, "-c:v", "copy", "-an", output_path],
        check=True,
    )


def add_audio(video_path: str, audio_path: str, output_path: str) -> None:
    """
    Replace or add audio track to a video file.

    Args:
        video_path: Source video (without or with audio; video track is kept).
        audio_path: Audio file to add.
        output_path: Output file.
    """
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", video_path,
            "-i", audio_path,
            "-map", "0:v", "-map", "1:a",
            "-c:v", "copy",
            "-c:a", "aac", "-b:a", "192k",
            "-shortest",
            output_path,
        ],
        check=True,
    )


# ---------------------------------------------------------------------------
# Watermarking
# ---------------------------------------------------------------------------

def add_text_watermark(
    input_path: str,
    output_path: str,
    text: str,
    fontsize: int = 28,
    opacity: float = 0.6,
    position: str = "bottom-right",
) -> None:
    """
    Burn a text watermark into the video.

    Args:
        input_path: Source video.
        output_path: Output video.
        text: Watermark text.
        fontsize: Font size in points.
        opacity: Text opacity (0.0 to 1.0).
        position: One of 'top-left', 'top-right', 'bottom-left', 'bottom-right'.
    """
    positions = {
        "top-left":     "x=20:y=20",
        "top-right":    "x=W-tw-20:y=20",
        "bottom-left":  "x=20:y=H-th-20",
        "bottom-right": "x=W-tw-20:y=H-th-20",
    }
    xy = positions.get(position, positions["bottom-right"])

    vf = (
        f"drawtext=text='{text}':"
        f"fontcolor=white@{opacity}:"
        f"fontsize={fontsize}:"
        f"{xy}"
    )

    subprocess.run(
        [
            "ffmpeg", "-y",
            "-i", input_path,
            "-vf", vf,
            "-c:v", "libx264", "-crf", "23",
            "-c:a", "copy",
            output_path,
        ],
        check=True,
    )


# ---------------------------------------------------------------------------
# Concatenation
# ---------------------------------------------------------------------------

def concatenate_videos(input_paths: list[str], output_path: str) -> None:
    """
    Concatenate multiple video files using the concat demuxer (stream copy).

    All inputs must have the same codec, resolution, and frame rate.

    Args:
        input_paths: Ordered list of video file paths.
        output_path: Output video file.
    """
    # Write a temporary concat list file
    list_path = "/tmp/ffmpeg_concat_list.txt"
    with open(list_path, "w") as f:
        for p in input_paths:
            abs_path = os.path.abspath(p)
            f.write(f"file '{abs_path}'\n")

    subprocess.run(
        [
            "ffmpeg", "-y",
            "-f", "concat",
            "-safe", "0",
            "-i", list_path,
            "-c", "copy",
            output_path,
        ],
        check=True,
    )


# ---------------------------------------------------------------------------
# GIF Creation
# ---------------------------------------------------------------------------

def create_gif(
    input_path: str,
    output_path: str,
    start: float = 0,
    duration: float = 5.0,
    fps: int = 15,
    width: int = 480,
) -> None:
    """
    Create an optimized GIF from a video clip.

    Args:
        input_path: Source video.
        output_path: Output GIF file path.
        start: Start time in seconds.
        duration: Duration in seconds.
        fps: Frames per second (10–20 recommended for GIF).
        width: GIF width in pixels.
    """
    filter_graph = (
        f"[0:v] fps={fps},scale={width}:-1,split [a][b];"
        "[a] palettegen [p];"
        "[b][p] paletteuse"
    )
    subprocess.run(
        [
            "ffmpeg", "-y",
            "-ss", str(start),
            "-t", str(duration),
            "-i", input_path,
            "-filter_complex", filter_graph,
            output_path,
        ],
        check=True,
    )


# ---------------------------------------------------------------------------
# Streaming
# ---------------------------------------------------------------------------

def stream_to_rtmp(
    input_path: str,
    rtmp_url: str,
    video_bitrate: str = "3000k",
    audio_bitrate: str = "160k",
) -> None:
    """
    Stream a local video file to an RTMP endpoint (e.g., YouTube, Twitch).

    Args:
        input_path: Source video file.
        rtmp_url: RTMP destination URL (e.g., rtmp://a.rtmp.youtube.com/live2/KEY).
        video_bitrate: Video bit rate.
        audio_bitrate: Audio bit rate.
    """
    subprocess.run(
        [
            "ffmpeg",
            "-re",
            "-i", input_path,
            "-c:v", "libx264",
            "-preset", "veryfast",
            "-b:v", video_bitrate,
            "-maxrate", video_bitrate,
            "-bufsize", str(int(video_bitrate.rstrip("k")) * 2) + "k",
            "-g", "60",
            "-keyint_min", "60",
            "-c:a", "aac",
            "-b:a", audio_bitrate,
            "-ar", "48000",
            "-f", "flv",
            rtmp_url,
        ],
        check=True,
    )


# ---------------------------------------------------------------------------
# Example usage
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    # Example: probe a file
    # info = probe_media("sample.mp4")
    # print(json.dumps(info, indent=2))

    # Example: transcode to H.264
    # transcode_to_h264("input.mov", "output.mp4", crf=22, preset="slow")

    # Example: extract thumbnail
    # extract_thumbnail("video.mp4", "thumb.jpg", timestamp=10.0)

    # Example: normalize podcast audio
    # normalize_loudness("raw_podcast.wav", "podcast_master.mp3", target_lufs=-16.0)

    print("FFmpeg Python helpers loaded. Import and use the functions above.")
