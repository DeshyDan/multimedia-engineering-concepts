"""
FFmpeg Media Analysis & Quality Metrics in Python
==================================================
Tools for analyzing media files and measuring encoding quality.

Usage:
    python media_analyzer.py input.mp4
    python media_analyzer.py --compare original.mp4 encoded.mp4

Requirements:
    - ffmpeg with libvmaf support (for VMAF analysis)
    - ffprobe
    - Python 3.10+
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass, field


# ---------------------------------------------------------------------------
# Data models
# ---------------------------------------------------------------------------

@dataclass
class StreamInfo:
    index: int
    codec_type: str
    codec_name: str
    extra: dict = field(default_factory=dict)


@dataclass
class VideoStreamInfo(StreamInfo):
    width: int = 0
    height: int = 0
    fps: str = ""
    pixel_format: str = ""
    bit_rate_kbps: float = 0.0
    color_space: str = ""
    color_transfer: str = ""
    color_primaries: str = ""


@dataclass
class AudioStreamInfo(StreamInfo):
    sample_rate: int = 0
    channels: int = 0
    channel_layout: str = ""
    bit_rate_kbps: float = 0.0
    sample_fmt: str = ""


@dataclass
class MediaInfo:
    path: str
    duration_sec: float
    size_bytes: int
    overall_bitrate_kbps: float
    format_name: str
    video_streams: list[VideoStreamInfo] = field(default_factory=list)
    audio_streams: list[AudioStreamInfo] = field(default_factory=list)


@dataclass
class QualityMetrics:
    psnr_y: float | None = None      # Y (luma) PSNR
    psnr_avg: float | None = None    # Average PSNR
    ssim_all: float | None = None    # SSIM overall
    vmaf: float | None = None        # VMAF score
    vmaf_min: float | None = None
    vmaf_max: float | None = None


# ---------------------------------------------------------------------------
# Media analysis
# ---------------------------------------------------------------------------

def _parse_fps(fps_str: str) -> str:
    """Evaluate a fractional fps string like '30000/1001' -> '29.97'."""
    if "/" in fps_str:
        num, den = fps_str.split("/")
        if int(den) > 0:
            return f"{int(num) / int(den):.3f}"
    return fps_str


def analyze(path: str) -> MediaInfo:
    """
    Probe a media file and return structured MediaInfo.

    Args:
        path: File path to analyze.

    Returns:
        MediaInfo dataclass with all stream details.
    """
    result = subprocess.run(
        [
            "ffprobe",
            "-v", "quiet",
            "-print_format", "json",
            "-show_format",
            "-show_streams",
            path,
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    data = json.loads(result.stdout)
    fmt = data.get("format", {})

    duration = float(fmt.get("duration", 0))
    size = int(fmt.get("size", 0))
    bitrate = float(fmt.get("bit_rate", 0)) / 1000
    format_name = fmt.get("format_name", "")

    video_streams: list[VideoStreamInfo] = []
    audio_streams: list[AudioStreamInfo] = []

    for stream in data.get("streams", []):
        idx = stream.get("index", 0)
        ctype = stream.get("codec_type", "")
        cname = stream.get("codec_name", "")
        raw_br = float(stream.get("bit_rate", 0)) / 1000

        if ctype == "video":
            video_streams.append(
                VideoStreamInfo(
                    index=idx,
                    codec_type=ctype,
                    codec_name=cname,
                    width=stream.get("width", 0),
                    height=stream.get("height", 0),
                    fps=_parse_fps(stream.get("r_frame_rate", "0/1")),
                    pixel_format=stream.get("pix_fmt", ""),
                    bit_rate_kbps=raw_br,
                    color_space=stream.get("color_space", ""),
                    color_transfer=stream.get("color_transfer", ""),
                    color_primaries=stream.get("color_primaries", ""),
                )
            )
        elif ctype == "audio":
            audio_streams.append(
                AudioStreamInfo(
                    index=idx,
                    codec_type=ctype,
                    codec_name=cname,
                    sample_rate=int(stream.get("sample_rate", 0)),
                    channels=stream.get("channels", 0),
                    channel_layout=stream.get("channel_layout", ""),
                    bit_rate_kbps=raw_br,
                    sample_fmt=stream.get("sample_fmt", ""),
                )
            )

    return MediaInfo(
        path=path,
        duration_sec=duration,
        size_bytes=size,
        overall_bitrate_kbps=bitrate,
        format_name=format_name,
        video_streams=video_streams,
        audio_streams=audio_streams,
    )


def print_media_info(info: MediaInfo) -> None:
    """Print a human-readable summary of media file information."""
    print(f"\n{'='*60}")
    print(f"File:       {info.path}")
    print(f"Format:     {info.format_name}")
    print(f"Duration:   {info.duration_sec:.2f} seconds "
          f"({_format_duration(info.duration_sec)})")
    print(f"File size:  {info.size_bytes / 1024 / 1024:.2f} MB")
    print(f"Bitrate:    {info.overall_bitrate_kbps:.0f} kbps")

    for v in info.video_streams:
        print(f"\n  Video Stream #{v.index}")
        print(f"    Codec:        {v.codec_name}")
        print(f"    Resolution:   {v.width}x{v.height}")
        print(f"    Frame rate:   {v.fps} fps")
        print(f"    Pixel format: {v.pixel_format}")
        if v.bit_rate_kbps:
            print(f"    Bit rate:     {v.bit_rate_kbps:.0f} kbps")
        if v.color_space:
            print(f"    Color space:  {v.color_space}")
        if v.color_transfer:
            print(f"    Transfer fn:  {v.color_transfer}")
        if v.color_primaries:
            print(f"    Primaries:    {v.color_primaries}")

    for a in info.audio_streams:
        print(f"\n  Audio Stream #{a.index}")
        print(f"    Codec:        {a.codec_name}")
        print(f"    Sample rate:  {a.sample_rate} Hz")
        print(f"    Channels:     {a.channels} ({a.channel_layout})")
        print(f"    Sample fmt:   {a.sample_fmt}")
        if a.bit_rate_kbps:
            print(f"    Bit rate:     {a.bit_rate_kbps:.0f} kbps")

    print(f"{'='*60}\n")


def _format_duration(seconds: float) -> str:
    h = int(seconds // 3600)
    m = int((seconds % 3600) // 60)
    s = seconds % 60
    return f"{h:02d}:{m:02d}:{s:05.2f}"


# ---------------------------------------------------------------------------
# Quality metrics (PSNR, SSIM, VMAF)
# ---------------------------------------------------------------------------

def measure_psnr_ssim(reference_path: str, distorted_path: str) -> QualityMetrics:
    """
    Measure PSNR and SSIM between a reference and distorted video.

    Args:
        reference_path: Original (reference) video file.
        distorted_path: Encoded/processed (distorted) video file.

    Returns:
        QualityMetrics with psnr_y, psnr_avg, ssim_all populated.
    """
    result = subprocess.run(
        [
            "ffmpeg",
            "-i", reference_path,
            "-i", distorted_path,
            "-filter_complex",
            "[0:v][1:v]psnr=stats_file=/tmp/psnr_stats.txt;"
            "[0:v][1:v]ssim=stats_file=/tmp/ssim_stats.txt",
            "-f", "null", "-",
        ],
        capture_output=True,
        text=True,
    )

    metrics = QualityMetrics()

    # Parse PSNR from stderr
    for line in result.stderr.splitlines():
        if "PSNR" in line and "average" in line.lower():
            parts = line.split()
            for i, p in enumerate(parts):
                if p.startswith("average:"):
                    try:
                        metrics.psnr_avg = float(p.split(":")[1])
                    except (ValueError, IndexError):
                        pass
                if p == "y:" and i + 1 < len(parts):
                    try:
                        metrics.psnr_y = float(parts[i + 1])
                    except ValueError:
                        pass

        if "SSIM" in line and "All:" in line:
            try:
                # Format: SSIM All:0.994321 (22.413310)
                all_part = line.split("All:")[1].strip()
                metrics.ssim_all = float(all_part.split()[0])
            except (IndexError, ValueError):
                pass

    return metrics


def measure_vmaf(
    reference_path: str,
    distorted_path: str,
    model_path: str = "vmaf_v0.6.1.json",
) -> QualityMetrics:
    """
    Measure VMAF score between a reference and distorted video.

    Requires FFmpeg compiled with libvmaf support.

    Args:
        reference_path: Original reference video.
        distorted_path: Encoded/processed video to evaluate.
        model_path: Path to VMAF model file.

    Returns:
        QualityMetrics with vmaf, vmaf_min, vmaf_max populated.
    """
    log_path = "/tmp/vmaf_results.json"

    subprocess.run(
        [
            "ffmpeg",
            "-i", reference_path,
            "-i", distorted_path,
            "-filter_complex",
            f"[0:v][1:v]libvmaf=model_path={model_path}:"
            f"log_path={log_path}:log_fmt=json:psnr=1:ssim=1",
            "-f", "null", "-",
        ],
        check=True,
        capture_output=True,
    )

    with open(log_path) as f:
        data = json.load(f)

    pooled = data.get("pooled_metrics", {})
    vmaf_data = pooled.get("vmaf", {})

    return QualityMetrics(
        vmaf=vmaf_data.get("mean"),
        vmaf_min=vmaf_data.get("min"),
        vmaf_max=vmaf_data.get("max"),
        psnr_y=pooled.get("psnr_y", {}).get("mean"),
        ssim_all=pooled.get("ssim", {}).get("mean"),
    )


# ---------------------------------------------------------------------------
# Loudness measurement
# ---------------------------------------------------------------------------

def measure_loudness(input_path: str) -> dict:
    """
    Measure EBU R128 integrated loudness of an audio or video file.

    Args:
        input_path: Media file to analyze.

    Returns:
        Dict with keys: integrated_lufs, true_peak_dbfs, loudness_range_lu.
    """
    result = subprocess.run(
        [
            "ffmpeg",
            "-i", input_path,
            "-af", "loudnorm=I=-14:LRA=11:TP=-1.5:print_format=json",
            "-f", "null", "-",
        ],
        capture_output=True,
        text=True,
    )

    output = result.stderr
    json_start = output.rfind("{")
    json_end = output.rfind("}") + 1

    if json_start < 0:
        return {}

    data = json.loads(output[json_start:json_end])
    return {
        "integrated_lufs": float(data.get("input_i", 0)),
        "true_peak_dbfs": float(data.get("input_tp", 0)),
        "loudness_range_lu": float(data.get("input_lra", 0)),
        "threshold": float(data.get("input_thresh", 0)),
    }


# ---------------------------------------------------------------------------
# CLI entrypoint
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Analyze media files and measure quality metrics",
    )
    parser.add_argument("input", help="Input media file to analyze")
    parser.add_argument(
        "--compare",
        metavar="DISTORTED",
        help="Compare input (reference) against a distorted/encoded file (PSNR/SSIM)",
    )
    parser.add_argument(
        "--loudness",
        action="store_true",
        help="Measure EBU R128 loudness statistics",
    )
    args = parser.parse_args()

    info = analyze(args.input)
    print_media_info(info)

    if args.compare:
        print(f"Measuring quality metrics vs: {args.compare}")
        metrics = measure_psnr_ssim(args.input, args.compare)
        print(f"\nQuality Metrics:")
        if metrics.psnr_y is not None:
            print(f"  PSNR (Y/luma):  {metrics.psnr_y:.2f} dB")
        if metrics.psnr_avg is not None:
            print(f"  PSNR (average): {metrics.psnr_avg:.2f} dB")
        if metrics.ssim_all is not None:
            print(f"  SSIM:           {metrics.ssim_all:.6f}")

    if args.loudness:
        print("Measuring loudness...")
        loudness = measure_loudness(args.input)
        print(f"\nLoudness Metrics (EBU R128):")
        print(f"  Integrated:    {loudness.get('integrated_lufs', 'N/A'):.1f} LUFS")
        print(f"  True Peak:     {loudness.get('true_peak_dbfs', 'N/A'):.1f} dBTP")
        print(f"  Loudness Range:{loudness.get('loudness_range_lu', 'N/A'):.1f} LU")


if __name__ == "__main__":
    main()
