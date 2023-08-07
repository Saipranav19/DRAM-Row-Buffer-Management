DRAM Row Buffer Management

Overview:

In the given lab assignment we use USIMM simulator to find the performance of the DRAM under
different page policies.

We have mainly 3 page policies that we are analysing using the simulator.

1)Open page policy: When we bring the row into the row buffer then if the next command is also a
load or store operation on the same row we just have to perform the column access , if there is row
buffer miss then we have to again perform precharge , act commands and bring the new row to the
row buffer and then perform the column access. This usually shows a significantly better
performance when there is exploitation of spatial locality significantly.

2)Closed page policy: A closed-page policy ensures that a DRAM page is closed immediately after
every read or write. This effectively eliminates both page-misses and page-hits, making every access
a page-empty. This may not allow for maximum memory bandwidth, but makes the access latency
predictable. This policy usually shows a significantly better performance when there are memory
request sequences with low access locality and/or low request rates.

3)Adaptive/hybrid page policy: It is intermediate policy of closed and open policy here we maintain
a timer to control the sense amplifiers . The timer is set to a predefined value when a row is
activated/reaccessed. Precharge command is issued issued when the timer reaches zero. It performs
significantly better when the memory request sequences whose request rate and access locality can
change dynamically.

Process:
1. For implementing the adaptive page policy as mentioned in the problem statement we
maintain a counter for which we make sure that the max value is 15 (as it is a 4 bit counter).
Now we keep on accessing multiple rows and columns. Now we keep updating the counter
as the program execution takes place everytime we are in a closed page policy and we
access a row which is different from the previous accessed row, we increase the counter by
1 unit. And if we access the same row we decrement the counter by one unit.

2. We have taken the upper threshold value as 11 and the lower threshold value as 6. This
means if the counter goes below the value 6, we switch to the open page policy, and if its
above 11 we switch to the closed page policy and this change is done by changing the
variable open., if open equal to 1, we mean the policy is closed page policy and if its zero it is
the open page policy.

3. Now to implement this we modify the scheduler.c file in the src folder of usimm simulator.
We initialise a 3 dimensional array
recent_colacc[MAX_NUM_CHANNELS]{MAX_NUM_RANKS][MAX_NUM_BANKS] and
initialise the values to zero and we also make a 3 d array of same dimensions called prev
which maintains the previously accessed rows. Now everytime we get a request, if we are in
a closed page policy we check the requested row is similar to the previously opened row and
if that is true, we get a row buffer hit, we continue. Else, as it is a miss we increase the
counter by 1.

4. If it was a closed policy, now we check the given request with the help of the other 3d array
prev and at the end of each iteration we update the parameters of the array. And if all the
parameters of the previous requests are same as the current one, we get a hit and we
decrement the counter by one, else continue. We also keep track of the variable open in
regard with the counter values going above the high threshold or below the low threshold
and significantly change the variable and adapt the necessary policy and implement the
modifications as stated above.

Observations:
OPEN PAGE POLICY:
Number of dram cycles is 316586849
Read latency(for channel 0) is 240.17499
Write latency (for channel 0) is 1996.36495

CLOSED PAGE POLICY:
Number of dram cycles is 302044069
Read latency(for channel 0) is 218.91037
Write latency (for channel 0) is 1793.30896

ADAPTIVE PAGE POLICY:
Number of dram cycles is 301517861
Read latency(for channel 0) is 214.17207
Write latency (for channel 0) is 1761.89428

We can observe that adaptive based policy has gone through the least number of dram cycles, so
now we modify the value of address mapping in the file 4channel.cfg present in the input folder of
the usimm simuolator and simulate for both the value of address mapping for the adaptive page
policy.
ADDRESS MAPPING VALUE: 0
Number of dram cycles is 301517861
ADDRESS MAPPING VALUE: 1
Number of dram cycles is 311973865

Conclusion:

From the above observations of the metrics generated through the simulations for different page
policies we can observe that adaptive page policy with address mapping 0 has performed
comparatively better than the others. And closed page policy has performed better than open page
policy. This could be due to various reasons as mentioned in the overview.
POLICY NO OF DRAM CYCLES READ LATENCY WRITE LATENCY
OPEN-PAGE 316586849 240.17499 1996.36495
CLOSE-PAGE 302044069 218.91037 1793.30896
ADAPTIVE 301517861 214.17207 1761.89428
ADDRESS MAPPING 0 ADDRESS MAPPING 1
Number of dram cycles is 301517861
(adaptive)
Number of dram cycles is 311973865
(adaptive)