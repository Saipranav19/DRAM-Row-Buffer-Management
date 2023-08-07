#include <stdio.h>
#include "utlist.h"
#include "utils.h"

#include "memory_controller.h"

extern long long int CYCLE_VAL;
int open=0;
int recent_colacc[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
int prev[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
long long int num_aggr_precharge = 0;
unsigned int count=8; //the intial value of counter is greater than lower threshold and higher than high threshold
void init_scheduler_vars()
{
	for(int i=0;i<MAX_NUM_CHANNELS;i++)
    {
        for(int j=0;j<MAX_NUM_RANKS;j++)
        {
            for(int k=0;k<MAX_NUM_BANKS;k++)
            {
                recent_colacc[i][j][k]=0;
            }
        }
    }
    for(int i=0;i<MAX_NUM_CHANNELS;i++)
    {
        for(int j=0;j<MAX_NUM_RANKS;j++)
        {
            for(int k=0;k<MAX_NUM_BANKS;k++)
            {
                prev[i][j][k]=0;
            }
        }
    }

	return;
}

// write queue high water mark; begin draining writes if write queue exceeds this value
#define HI_WM 40

// end write queue drain once write queue has this many writes in it
#define LO_WM 20

#define LO_TH 7
#define HI_TH 12
// 1 means we are in write-drain mode for that channel
int drain_writes[MAX_NUM_CHANNELS];

/* Each cycle it is possible to issue a valid command from the read or write queues
   OR
   a valid precharge command to any bank (issue_precharge_command())
   OR 
   a valid precharge_all bank command to a rank (issue_all_bank_precharge_command())
   OR
   a power_down command (issue_powerdown_command()), programmed either for fast or slow exit mode
   OR
   a refresh command (issue_refresh_command())
   OR
   a power_up command (issue_powerup_command())
   OR
   an activate to a specific row (issue_activate_command()).

   If a COL-RD or COL-WR is picked for issue, the scheduler also has the
   option to issue an auto-precharge in this cycle (issue_autoprecharge()).

   Before issuing a command it is important to check if it is issuable. For the RD/WR queue resident commands, checking the "command_issuable" flag is necessary. To check if the other commands (mentioned above) can be issued, it is important to check one of the following functions: is_precharge_allowed, is_all_bank_precharge_allowed, is_powerdown_fast_allowed, is_powerdown_slow_allowed, is_powerup_allowed, is_refresh_allowed, is_autoprecharge_allowed, is_activate_allowed.
   */

void schedule(int channel)
{
	request_t * rd_ptr = NULL;
	request_t * wr_ptr = NULL;


	// if in write drain mode, keep draining writes until the
	// write queue occupancy drops to LO_WM
	if (drain_writes[channel] && (write_queue_length[channel] > LO_WM)) {
	  drain_writes[channel] = 1; // Keep draining.
	}
	else {
	  drain_writes[channel] = 0; // No need to drain.
	}

	// initiate write drain if either the write queue occupancy
	// has reached the HI_WM , OR, if there are no pending read
	// requests
	if(write_queue_length[channel] > HI_WM)
	{
		drain_writes[channel] = 1;
	}
	else {
	  if (!read_queue_length[channel])
	    drain_writes[channel] = 1;
	}
    //checking the counter value and switching the counter to closed policy if the counter is higher than the high threshold
    //or changing it back to open page policy if it is less than the lower threshold
    if(count<LO_TH)
     open=0;
    if(count>HI_TH)
     open=1;

	// If in write drain mode, look through all the write queue
	// elements (already arranged in the order of arrival), and
	// issue the command for the first request that is ready
   if(open==1)
   {
        // If in write drain mode, look through all the write queue
        // elements (already arranged in the order of arrival), and
        // issue the command for the first request that is ready	
        if(drain_writes[channel])
        {

            LL_FOREACH(write_queue_head[channel], wr_ptr)
            {
                if(wr_ptr->command_issuable)
                {
                    //check the command that is going to be issued so that to verify whether we can close the bank or not
                    //if the bank is going to be issued the coloumn access command then only it can be closed
                    if(wr_ptr->next_command == ACT_CMD || wr_ptr->next_command == PRE_CMD)
                    {
                        recent_colacc[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank]=0;
                    }
                    if(wr_ptr->next_command== COL_WRITE_CMD)
                    {
                        recent_colacc[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank]=wr_ptr->dram_addr.row+1;
                        if(prev[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank]==wr_ptr->dram_addr.row+1)
                        {
                            if(count==0)
                            {
                                count=0;
                            }
                            else
                            {
                                count-=1;
                            }
                        }
                    }
                    issue_request_command(wr_ptr);
                    break;
                }
            }
            return;
        }

        // Draining Reads
        // look through the queue and find the first request whose
        // command can be issued in this cycle and issue it 
        // Simple FCFS 
        if(!drain_writes[channel])
        {
            //following the same process as above but replacing the write pointer with read pointer
            LL_FOREACH(read_queue_head[channel],rd_ptr)
            {
                if(rd_ptr->command_issuable)
                {
                    //check the command that is going to be issued so that to verify whether we can close the bank or not
                    //if the bank is going to be issued the coloumn access command then only it can be closed
                    if(rd_ptr->next_command == ACT_CMD || rd_ptr->next_command == PRE_CMD)
                    {
                        recent_colacc[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank]=0;
                    }
                    if(rd_ptr->next_command== COL_READ_CMD)
                    {
                        recent_colacc[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank]=rd_ptr->dram_addr.row+1;
                        if(prev[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank]==rd_ptr->dram_addr.row+1)
                        {
                            if(count==0)
                            {
                                count=0;//lower saturation
                            }
                            else
                            {
                                count-=1;
                            }
                        }
                    }
                    issue_request_command(rd_ptr);
                    break;
                }
            }
        }

        //no command issued in the cycle we should perform precharge command
        if(!command_issued_current_cycle[channel])
        { 
            for(int i=0;i<MAX_NUM_RANKS;i++)
            {
                for(int j=0;j<MAX_NUM_BANKS;j++)
                {
                    if(recent_colacc[channel][i][j])
                    {
                        //check whether the bank is allowed for a precharge
                        if(is_precharge_allowed(channel,i,j))
                        {
                            if(issue_precharge_command(channel,i,j))
                            {
                                num_aggr_precharge++;
                                prev[channel][i][j]=recent_colacc[channel][i][j];
                                recent_colacc[channel][i][j]=0;
                            }
                        }
                    }
                }
            }
        }
   }
   else
   {
     if(drain_writes[channel])
     {
         LL_FOREACH(write_queue_head[channel],wr_ptr)
         {
             if(wr_ptr->command_issuable)
             {
                 if(wr_ptr->next_command==COL_WRITE_CMD)
                 {
                     recent_colacc[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank]=wr_ptr->dram_addr.row+1;  
                 }
                 if(dram_state[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank].active_row!=wr_ptr->dram_addr.row)
                 {
                     if(count==15)
                     {
                         count=15;//upper saturation of 4 bit counter
                     }
                     else
                     {
                         count+=1;
                     }
                 }
                 issue_request_command(wr_ptr);
                 break;
             }
         }
         return;
     }
     if(!drain_writes[channel])
     {
         LL_FOREACH(read_queue_head[channel],rd_ptr)
         {
             if(rd_ptr->command_issuable)
             {
                 if(rd_ptr->next_command==COL_READ_CMD)
                    recent_colacc[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank]=rd_ptr->dram_addr.row+1;
                 if(dram_state[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank].active_row!=rd_ptr->dram_addr.row)
                 {
                     if(count==15)
                     {
                         count=15;//upper satiration of 4 bit counter
                     }
                     else
                     {
                         count+=1;
                     }
                 }
                 issue_request_command(rd_ptr);
                 break;
             }
         }
         return;
     }
   }
}

void scheduler_stats()
{
    	if(open)
         printf("Number of aggressive precharges: %lld\n",num_aggr_precharge);
}

