# **Iris**

![Iris GUI](https://u.pone.rs/nraiobcg.PNG "Iris GUI")

## Overview

**Iris** is a dynamic convolution reverb that blends up to eight impulse responses simultaneously in real time, driven by the position of a point listener on an interactive 2D spatial map. As you move through the space, the reverb character morphs smoothly between the surrounding IRs, weighted by proximity. To achieve time-variant, artifact-free IR switching at low latency, Iris implements **TVOLAP (Time-Variant Overlap-Add in Partitions)**, a fast convolution algorithm that supports seamless mid-block IR coefficient updates. See [Jaeger et al. (2023)](https://arxiv.org/pdf/2310.00319) for the underlying paper.

## Features

- **Simultaneous convolution** — Up to eight IRs convolved and mixed per block, weights updated per-partition
- **Spatial proximity mapping** — 2D map where position and field indicator positions determine IR blending weights; supports several deterministic and stochastic motion patterns
- **Smooth IR swapping** — crossfade on IR change, auto-swap mode per IR slot with configurable swap interval
- **IR management** — load from directory with filename filtering; random selection within a filtered set

## Installation

Navigate to [Releases](https://github.com/VEGAnonymous/Iris/releases) and download the latest stable version in the format of choice. *Currently only standalone and VST3 builds are supported.*

Building manually requires [JUCE 7+](https://juce.com/) and a C++17 compiler. Open `Iris.jucer` in Projucer, export to an IDE of choice, and build.

## License

GPL-3.0 — see [LICENSE](LICENSE).