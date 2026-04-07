---
layout: default
title: "Project 1"
---

# Project 1 Proposal: Modular Signal Plotter


## Concept

This project is inspired by the fact that Stepdance itself was developed from ideas rooted in modular synthesis.
Building on this connection, the project proposes a system that extends this relationship: instead of using modular synthesis to generate sound, it uses modular signals to generate motion.
The plotter translates electrical signals from a modular system into movement, treating control voltage as a source of motion energy rather than sound. Inspired by modular patching, the same signal can take on different roles—such as oscillation, modulation, or accumulation—depending on how it is used within the system.
Rather than visualizing sound directly, the project focuses on how signal structures can shape behavior, resulting in drawings that evolve through oscillation, variation, and repetition

## Core Idea
**One signal, multiple behaviors**

- A single input signal (LFO or CV) is used
- The system reinterprets this signal in different ways
- Each interpretation produces a different drawing style
“Sound has 4 properties, pitch, loudness, timbre and duration, and silence has one – duration.” For motion in plotters, similar happens on pen up (sound) and pen down. For the motion control of plotter on paper, the properties that matter are either

1. The starting point, end point, length of the path, and the shape of the path. 
2. The starting point, the duration, the direction of movement at particular time of duration, the speed of the movement at particular time of the duration

The difference lies in whether we focus on delivering the final outcome on the paper, which makes a paint stroke a state invariant shape, or focus on the painting process of the plotter, make it more like a machine performance and have the paper painting only as a side profile of the process.

## System Structure
Modular CV (LFO) 
        ↓
Voltage scaling (safe range)
        ↓
Stepdance (Teensy input A1)
        ↓
Motion mapping (code)
        ↓
Plotter drawing


## Design

Explain your design process. What choices did you make and why?

## Implementation

Describe how you implemented your project using the StepDance library.

### Hardware Setup

Describe your hardware configuration.

![Hardware setup photo](assets/placeholder.jpg)

### Code Overview

Highlight key parts of your code and explain your approach:

```cpp
// Paste and explain relevant code snippets here
```

## Results

Show your project in action. Embed a video of it working:

<iframe width="560" height="315" src="https://www.youtube.com/embed/VIDEO_ID" frameborder="0" allowfullscreen></iframe>

*Replace the iframe above with your actual video URL, or use a local video:*

<!--
<video width="560" controls>
  <source src="assets/demo-video.mp4" type="video/mp4">
</video>
-->

## Reflection

What did you learn? What would you do differently?
