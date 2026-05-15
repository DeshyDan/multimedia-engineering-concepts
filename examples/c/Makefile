# FFmpeg C Examples — Makefile
#
# Builds all C examples that use the FFmpeg libraries directly.
# Requires pkg-config and FFmpeg development headers.
#
# References:
#   pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/
#   FFmpeg build: https://trac.ffmpeg.org/wiki/CompilationGuide
#
# Install FFmpeg dev headers:
#   Ubuntu/Debian: sudo apt install libavcodec-dev libavformat-dev \
#                                   libavfilter-dev libavutil-dev libswscale-dev
#   macOS:         brew install ffmpeg
#   Fedora/RHEL:   sudo dnf install ffmpeg-devel
#
# Build all:     make
# Build one:     make probe
# Clean:         make clean

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 $(shell pkg-config --cflags libavformat libavcodec libavfilter libavutil libswscale)
LDFLAGS := $(shell pkg-config --libs   libavformat libavcodec libavfilter libavutil libswscale)

TARGETS := probe remux decode_video encode_video transcode filtering

.PHONY: all clean

all: $(TARGETS)

probe: probe.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

remux: remux.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

decode_video: decode_video.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

encode_video: encode_video.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

transcode: transcode.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

filtering: filtering.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)
