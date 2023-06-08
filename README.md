# HS-LAUNCHER
The launcher component of Hellscythe. Note that this was part of a larger project and hence i didn't bother collecting all solution files, so you might not be able to compile this. Once i have time i'll publish some of the other components (launcher,website,backend,netsdk).

# Intro
You can check out my other repo `HS-KMON` if you'd like to hear the backstory of Hellscythe.

# Contents
Following components are included in this repo:
  - HSOVERLAY | Dynamic library which contains actual launcher logic and functionality.
  - HSRUNNER | Stub executable which hijacks a `rundll32.exe` process, gets execution for HSOVERLAY.
