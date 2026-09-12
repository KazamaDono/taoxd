# Rules of Engagement

This repository teaches offensive techniques for a defensive purpose: to understand how software fails so you can find bugs, build detections, and harden systems. That knowledge is dual-use, and how you apply it is on you.

**Three rules, no exceptions:**

1. **Authorization is binary.** Only run these techniques against systems you own or have explicit, written permission to test, within a defined scope. "Probably fine" is not authorization.
2. **Do no harm.** Even when authorized: don't destroy data, don't exceed scope, and report what you find through proper channels. See the disclosure guidance in Appendix D of the book.
3. **Stay in the lab.** Every target in this repo is a deliberately vulnerable teaching artifact or a version-pinned lab binary. Run them inside the provided container, or for kernel work a throwaway VM — never on a host or network you care about, and never against third-party software or services.

The vulnerable programs here are **intentionally** insecure. Do not deploy them, copy their patterns into real software, or expose them to any network.

If you are unsure whether an action is permitted, the answer is **no**.
