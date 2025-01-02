# ECC-disk

This is a just-for-fun project I made in a couple of days. I got a couple of Chinese counterfeit micro-SD cards from a friend (RIP money got scammed lol) and they are etched with not only fake capacities but also numerous bad sectors. (The chips were probably sent to "recycling facilities" run by Chinese fraudsters and turned into scam products on Chinese E-commerce platforms.) So, I decided to use software ECC to further "recycle" those junk SD cards and see how long they can withstand my ruthless torture.  
  
Since this is a just-for-fun project, the code quality is abysmal. I didn't bother to take care of fixed-size buffers and race conditions. Expect outrageous bugs and vulnerabilities that may make you curse about how stupid I am.  
  
Libraries (not made by me) used/modded:  
 1. [RSCode by hqm](https://github.com/hqm/rscode) under `/rscode/`
 2. [WinSPD by billziss](https://github.com/winfsp/winspd) under `/winspd/`
 3. [This StackOverflow snippet](https://stackoverflow.com/a/18553747)
  
Btw, please do NOT use this in ANY production environment. You should just throw broken storage devices away anyway, unless your goal is to grind them for fun. Also, run this ONLY in VMs. I am not responsible for any losses should the code be so bad that it break things.  
  
License: GPLv3 (also used by both libraries)  
  
Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)  
