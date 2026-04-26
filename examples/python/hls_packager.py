"""
FFmpeg HLS Packaging Example in Python
=======================================
Generates a multi-bitrate HLS package from a single source file.

Usage:
    python hls_packager.py input.mp4 ./output_hls

Requirements:
    - ffmpeg with libx264 and fdk_aac/aac support
    - Python 3.10+
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from dataclasses import dataclass


@dataclass
class RenditionConfig:
    """Defines a single HLS rendition (resolution + bit rate)."""

    name: str
    width: int
    height: int
    video_bitrate_kbps: int
    audio_bitrate_kbps: int
    max_rate_multiplier: float = 1.07


# Default multi-bitrate ladder
DEFAULT_RENDITIONS: list[RenditionConfig] = [
    RenditionConfig("1080p", 1920, 1080, 4500, 192),
    RenditionConfig("720p",  1280,  720, 2500, 128),
    RenditionConfig("480p",   854,  480, 1000,  96),
    RenditionConfig("360p",   640,  360,  500,  64),
]


def build_hls_command(
    input_path: str,
    output_dir: str,
    renditions: list[RenditionConfig],
    segment_duration: int = 6,
    use_fmp4: bool = True,
    keyframe_interval: int = 60,
) -> list[str]:
    """
    Build the FFmpeg command for multi-bitrate HLS packaging.

    Args:
        input_path: Source video file.
        output_dir: Output directory for HLS segments and playlists.
        renditions: List of renditions to produce.
        segment_duration: HLS segment duration in seconds.
        use_fmp4: Use fragmented MP4 segments (recommended) vs. MPEG-TS.
        keyframe_interval: Fixed keyframe interval (should be multiple of segment_duration * fps).

    Returns:
        FFmpeg command as a list of strings.
    """
    n = len(renditions)

    # Build the video split filter
    split_inputs = "".join(f"[v{i}]" for i in range(n))
    filter_parts = [f"[0:v]split={n}{split_inputs}"]

    for i, r in enumerate(renditions):
        filter_parts.append(f"[v{i}]scale={r.width}:{r.height}[vout{i}]")

    filter_complex = "; ".join(filter_parts)

    cmd = ["ffmpeg", "-y", "-i", input_path, "-filter_complex", filter_complex]

    # Video streams
    for i, r in enumerate(renditions):
        bv = f"{r.video_bitrate_kbps}k"
        maxrate = f"{int(r.video_bitrate_kbps * r.max_rate_multiplier)}k"
        bufsize = f"{r.video_bitrate_kbps * 2}k"
        cmd += [
            "-map", f"[vout{i}]",
            f"-c:v:{i}", "libx264",
            f"-b:v:{i}", bv,
            f"-maxrate:v:{i}", maxrate,
            f"-bufsize:v:{i}", bufsize,
            f"-preset:v:{i}", "fast",
        ]

    # Audio streams (one per rendition)
    for i, r in enumerate(renditions):
        cmd += [
            "-map", "0:a",
            f"-c:a:{i}", "aac",
            f"-b:a:{i}", f"{r.audio_bitrate_kbps}k",
            f"-ar:{i}", "48000",
        ]

    # Global x264 options for constant keyframe interval
    cmd += [
        "-x264opts", f"keyint={keyframe_interval}:min-keyint={keyframe_interval}:no-scenecut",
        "-g", str(keyframe_interval),
        "-keyint_min", str(keyframe_interval),
        "-sc_threshold", "0",
    ]

    # HLS output options
    segment_ext = "mp4" if use_fmp4 else "ts"
    segment_type_flag = ["-hls_segment_type", "fmp4"] if use_fmp4 else []

    master_pl = os.path.join(output_dir, "master.m3u8")
    var_stream_map = " ".join(f"v:{i},a:{i}" for i in range(n))
    segment_filename = os.path.join(output_dir, "stream_%v", f"seg_%03d.{segment_ext}")
    variant_playlist = os.path.join(output_dir, "stream_%v", "playlist.m3u8")

    cmd += [
        "-f", "hls",
        "-hls_time", str(segment_duration),
        "-hls_playlist_type", "vod",
        "-hls_flags", "independent_segments",
        *segment_type_flag,
        "-master_pl_name", master_pl,
        "-var_stream_map", var_stream_map,
        "-hls_segment_filename", segment_filename,
        variant_playlist,
    ]

    return cmd


def package_hls(
    input_path: str,
    output_dir: str,
    renditions: list[RenditionConfig] | None = None,
    segment_duration: int = 6,
) -> None:
    """
    Package a video file as multi-bitrate HLS.

    Args:
        input_path: Source video.
        output_dir: Directory to write HLS output.
        renditions: Custom rendition configurations (default ladder used if None).
        segment_duration: Duration of each HLS segment in seconds.
    """
    if renditions is None:
        renditions = DEFAULT_RENDITIONS

    # Create output subdirectories for each rendition
    for i in range(len(renditions)):
        os.makedirs(os.path.join(output_dir, f"stream_{i}"), exist_ok=True)

    cmd = build_hls_command(
        input_path,
        output_dir,
        renditions,
        segment_duration=segment_duration,
    )

    print(f"Packaging HLS with {len(renditions)} renditions...")
    print(f"Command: {' '.join(cmd)}\n")

    subprocess.run(cmd, check=True)
    print(f"\nHLS output written to: {output_dir}")
    print(f"Master playlist: {os.path.join(output_dir, 'master.m3u8')}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Package a video file as multi-bitrate HLS",
    )
    parser.add_argument("input", help="Input video file path")
    parser.add_argument("output", help="Output directory for HLS package")
    parser.add_argument(
        "--segment-duration", type=int, default=6,
        help="HLS segment duration in seconds (default: 6)",
    )
    args = parser.parse_args()

    if not os.path.isfile(args.input):
        print(f"Error: input file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    package_hls(args.input, args.output, segment_duration=args.segment_duration)


if __name__ == "__main__":
    main()
