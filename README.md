ExpectiMax2048
==============

This is the C++ part of my 2048 AI. The HTML/JS part is here:

https://github.com/MaartenBaert/2048

And you can try it here:

https://maartenbaert.github.io/2048/

This code is a bit messy and there is no documentation, but it works and it's pretty good. I'm not really doing anything with this code anymore, but if you want to experiment with it, go ahead. If you can make an improvement that's demonstrably better (by running a batch test and comparing it to my best results), send me a pull request and I will merge it.

The results
-----------

* Highest score seen in-game: [344864](https://www.youtube.com/watch?v=L4gRsNmpYmc)
* Highest score in simulator: 382036

I haven't seen any human get a similar score in 'vanilla' 2048. If you have found one (ideally with video as proof), let me know.

I have found [one other AI](https://stackoverflow.com/questions/22342854/what-is-the-optimal-algorithm-for-the-game-2048/22498940#22498940) that uses pretty much the same algorithm (but with different heuristics and optimizations) and very similar results (just slightly worse than this one). I have not found any AI that is significantly better than this one.

Algorithm
---------

This program started as a simple minimax-based AI, but I quickly realized that I could get much more accurate results by using a model based on probabilities rather than simply assuming that the opponent (the computer, essentially) will always place blocks in the worst position possible (this is called the 'expectimax' algorithm - I kept the old name because I was lazy). Because of this change, the AI can accurately calculate the risks and rewards of each move and make a better decision. This turns out to be particularly important at later stages of the game, because there are moments where the board will unavoidably get almost completely filled, and at these points it is not possible to find a strategy that is guaranteed to be 100% safe. In these cases, traditional minimax will fail.

The probability-based algorithm is significantly slower than Minimax, mainly because it is incompatible with alpha-beta pruning. I usually limit the search depth to six moves (for each side). The heuristic becomes really important with such a limited depth.

The current heuristic is the result of trial and error, and lots of tuning. I'm sure it can be improved a lot more, but it's not easy to find good rules that are also fast enough (the heuristic is evaluated thousands of times). Rules that work well for human players don't necessarily work well for this AI. The popular 'power square' tactic ('keep your highest square in a corner at all times') actually appears to make this AI perform worse. This isn't that surprising actually, since there is probably no human player that's as good as this AI. Most human tactics are overly conservative and rely a lot on luck, and this simply doesn't work at later stages of the game. Remember, the goal is *not* to get the highest score ever, but to *consistently* get high scores.

The expectimax algorithm itself is about as good as it can get, but the speed could probably be improved with some more optimizations, such as better caching or a smarter representation of the board. You can probably improve things even more if you optimize separately for each depth. If you can make it about ten times faster, you can increase the search depth from 6 to 7 which will definitely improve the results a lot (just compare depth 4, 5 and 6 and you will see the trend).

Usage
-----

This AI can either be compiled as a stand-alone C++ program for testing purposes (e.g. to test new algorithms or tune the parameters), or converted to Javascript with Emscripten (so it can be embedded in the 2048 webpage).

There are some tests in main.cpp that can be configured with a few #defines at the top. There are two main ways to test the AI:

* *Batch test mode:* This mode simulates a fixed number of games with the default parameters. At the end, it prints the list of scores. The list can be copied to analyze/plot.py for analysis.

* *Tuning mode:* This mode simulates games as part of a genetic algorithm that tries to improve the parameters. At the end, it prints the new suggested parameters and a list of scores. The list of scores can be copied to analyze/tune.py to analyze convergence. Convergence is rather slow and the scores are extremely noisy, so you won't see any measurable improvement unless you simulate a huge number of games. I usually run the tuning algorithm at a lower-than-normal search depth to make it faster, but this probably produces biased results so it's not ideal. If you want to run the tuning at the full search depth (6), go ahead, but it will take at least a few days even with a powerful CPU.

Both test modes are multithreaded and significantly faster than the Javascript version. Simulation time depends a lot on the number of moves. On an Intel Core i7-4770 CPU, one move takes 40-50ms at search depth 6 (when there are 8 simulations running in parallel). A typical game lasts for about 7000 moves, so I can simulate one game in about 350 seconds. Since there are 8 simulations running in parallel, I get one new result roughly every 40 seconds.

Compiling
---------

There is no proper build system, I use Qt Creator to compile the C++ program. You can also compile it directly with GCC with this command:

    g++ -pthread -std=c++0x -Wl,-O3 -O3 -flto -fuse-linker-plugin -DNDEBUG *.cpp -o MiniMax2048

In theory it should compile on all platforms, but I only tested it on Linux (64-bit).

The JS version can be compiled with the 'compile' script in the emscripten directory, but you will have to change some paths to make it work.
