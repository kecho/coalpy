---
permalink: /tutorial-intro
title: "Introduction"
layout: "single"
sidebar:
  nav: "tutorialbar"
---

Welcome to __CoalPy__ ! The __CoalPy__ library stands for _Compute Abstraction Layer for Python_. This library provides a simple and straight forward way to write GPU based 
applications.

![image](images/brdf-scene.png?raw=true "Example CoalPy app using ImGui, GPU compute based ray tracing and multiple windows.")

# Motivation

In an age when GPUs are in such high demand __CoalPy__ tries to simplify CPU setup for GPU programming. This library offers a set of python abstractions that operate over _DirectX12_ or _Vulkan_ backends.

__CoalPy__ allows the user to focus on writing GPU compute shaders for any graphics card compliant with _DirectX12_ or _Vulkan_.
The user prepares a set of simple command lists containing compute shader kernels written in _hlsl_. These shaders can write to a texture or buffer, and propagate this output to more shaders creating a compute graph.
The shading language used is _hlsl_ and is one of the most famous professional programming languages used for GPU programming in the gaming industry. It provides a lot of interesting features not present in __Cuda__ and other Python GPU frameworks.
The idea is to let users write powerful _hlsl_ to solve problems without having to write thousands of lines in _DirectX12_ or _Vulkan_ and not having to learn some obscure Python sub language. Write GPU code how it was meant to!

# Features

* **Live editing of compute _hlsl_ shaders.** You can change shader code live while the application is running.
* **Full imgui integration.** Thats right! write _DearImGUI_ code directly in python to provide a rich user interface experience.
* **Live editing of texture data.** Edit texture data while application runs, 
* **Access to advanced GPU features** Easy access to advanced modern GPU compute features such as _bindless_ and _indirect_ dispatch.
* **Easy to use window management system**
* **Profiling tools** __CoalPy__ comes with an internal profiler built in coalpy to profile GPU code. 
* **Multiple backend support** Coalpy can run over DirectX12 or Vulkan backend on Windows. Linux is still under development.

The above features will allow you to write rich and interesting applications with a major focus on GPU high performant code.

