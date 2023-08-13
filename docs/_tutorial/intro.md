---
permalink: /tutorial-intro
title: "Introduction"
layout: "single"
sidebar:
  nav: "tutorialbar"
---

Welcome to __CoalPy__ ! __CoalPy__ library stands for _Compute Abstraction Layer for Python_. This library provides a simple and straight forward way to write GPU based 
applications.

![image](images/brdf-scene.png?raw=true "Example CoalPy app using ImGui, GPU compute based ray tracing and multiple windows.")

# Motivation

In an age when GPUs are in such high demand __CoalPy__ tries to simplify CPU setup for GPU programming. This library offers a set of python abstractions that operate under _DirectX12_ or _Vulkan_ backends.

__CoalPy__ allows the user to focus on writing GPU compute shaders for any graphics card compliant with _DirectX12_ or _Vulkan_.
The user prepares a set of simple command lists containing compute shader kernels written in _hlsl_. These shaders can write to a texture or buffer, and propagate this output to more shaders creating a compute graph.
The shading language used is _hlsl_ and is one of the most famous professional programming languages used for GPU programming in the gaming industry. It provides a lot of interesting features not present in __Cuda__ and other Python GPU frameworks.
The idea is to let users write powerful _hlsl_ to solve problems without having to write thousands of lines in _DirectX12_ or _Vulkan_.

The skills built on top of __CoalPy__ will translate nicely to any job or problem requiring a GPU compute shader!

# Features

* **Live editing of compute _hlsl_ shaders.** You can change shader code live while the application is running.
* **Full imgui integration.** Thats right! write _DearImGUI_ code directly in python to provide a rich user interface experience.
* **Live editing of texture data.** Thats right! edit texture data while application runs, 
* **Access to advanced GPU features** such as _bindless_ and _indirect_ dispatch.**
* **Partial experimental Vulkan Backend Support** which is still under development.
* **Easy to use window management system**
* **Profiling tools** __CoalPy__ comes with an internal profiler built in coalpy to profile GPU code. 

The above features scratch just the surface, and will allow you to write rich and interesting applications with a major focus on GPU high performant code.

