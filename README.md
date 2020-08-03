MultiDrawIndirect allows GPU-local culling by setting the draw count of individual commands in the indirect buffer to 0. However, this leaves "empty draws" or "holes" in the command buffer, which can potentially still incur similar costs to a real draw, depending on the driver / GPU firmware. This program very crudely measures the time difference between a full (unculled) dispatch, and one where most of the draws have been neutralized but remain present.

In order to remedy this overhead, GPU-local compaction passes should be performed that cull the aforementioned holes and produce a tightly packed command buffer containing only draws that actually do something (count >= 1).

## "Why does the name contain 'Compaction' if it's not being performed here?"
Because I was too stupid to think of a good name. I might actually test the performance of compaction passes one day if I get bored.

## "Why are there some funny looking parts of the code?"
I usually try to mark code that should really be rewritten later, by writing it in an obnoxious, cursed way. Marker and reminder in one.

## "This is a garbage benchmark, you're just measuring two data points and not even aggregating averages / medians / std deviations"
Yep. I realized that as soon as I tested it and my machine showed literally no difference in execution time between a normal and an 穴だらけ dispatch, that any further investigation would just be wasted effort for the time being.

## Further reading:
Slide 32 deals with compaction passes:  
https://www.slideshare.net/gwihlidal/optimizing-the-graphics-pipeline-with-compute-gdc-2016

Compaction passes might benefit from running as graphics tasks (interleaved with rendering) as opposed to compute:  
https://on-demand.gputechconf.com/gtc/2016/presentation/s6138-christoph-kubisch-pierre-boudier-gpu-driven-rendering.pdf#page=33
