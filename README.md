MultiDrawIndirect allows GPU-local culling by setting the draw count of individual commands in the indirect buffer to 0. However, this leaves "empty draws" or "holes" in the command buffer, which can potentially still incur similar costs to a real draw, depending on the driver / GPU firmware. This program very crudely measures the time difference between a full (unculled) dispatch, and one where most of the draws have been neutralized but remain present.

In order to remedy this overhead, GPU-local compaction passes should be performed that cull the aforementioned holes and produce a tightly packed command buffer containing only draws that actually do something (count >= 1).

## Further reading:
Slide 32 deals with compaction passes:  
https://www.slideshare.net/gwihlidal/optimizing-the-graphics-pipeline-with-compute-gdc-2016

Compaction passes might benefit from running as graphics tasks (interleaved with rendering) as opposed to compute:  
https://on-demand.gputechconf.com/gtc/2016/presentation/s6138-christoph-kubisch-pierre-boudier-gpu-driven-rendering.pdf#page=33
