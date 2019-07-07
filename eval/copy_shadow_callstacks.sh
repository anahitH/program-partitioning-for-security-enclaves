#!/bin/bash

for d in 10/*/* ; do cp shadow_call_stack.txt "$d"; done
for d in 25/*/* ; do cp shadow_call_stack.txt "$d"; done
for d in 50/*/* ; do cp shadow_call_stack.txt "$d"; done
for d in 50/*/* ; do cp shadow_call_stack.txt "$d"; done

for d in expert-knowledge/* ; do cp shadow_call_stack.txt "$d"; done
