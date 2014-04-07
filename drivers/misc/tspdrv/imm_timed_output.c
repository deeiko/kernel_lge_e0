#include<linux/kernel.h>
#include<linux/module.h>
#include"../../staging/android/timed_output.h"
#include<linux/hrtimer.h>
#include<linux/sched.h>
#include"imm_timed_output.h"
#define max_timeout_ms 15000

static struct hrtimer vib_timer;
static unsigned int vib_state;
static struct work_struct vibrator_work;
extern int32_t ImmVibeSPI_ForceOut_AmpEnable(u_int8_t);
extern int vibrator_pwm_set(int,int);
extern int32_t ImmVibeSPI_ForceOut_AmpDisable(u_int8_t);

static void ImmVibeSPI_Control(struct work_struct *work)
{
	if(vib_state) {
		ImmVibeSPI_ForceOut_AmpEnable(0);
		vibrator_pwm_set(1,127);
	}
	else {
		ImmVibeSPI_ForceOut_AmpDisable(0);
	}
}

static void tspdrv_vib_enable(struct timed_output_dev *dev, int value)
{
	
	hrtimer_cancel(&vib_timer);

	if(value == 0) {
		vib_state =  0; //Turn Off the vibrator
		schedule_work(&vibrator_work);
 	}
	else {
		value = (value > max_timeout_ms ? max_timeout_ms : value);
        /*                                                            */
		if(value < 10)
            value = 10;
        /*                                                          */

		vib_state = 1;
		schedule_work(&vibrator_work);

		hrtimer_start(&vib_timer,
			ktime_set(value/ 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}

}

static int tspdrv_vib_get_time(struct timed_output_dev *dev)
{
	if(hrtimer_active(&vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&vib_timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_sec / 1000;
	}	

	return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	vib_state = 0;
	schedule_work(&vibrator_work);
	return HRTIMER_NORESTART;
}
static struct timed_output_dev timed_dev = {
	.name = "vibrator",
	.get_time = tspdrv_vib_get_time,
	.enable	= tspdrv_vib_enable,
}; 

void __init ImmVibe_timed_output(void)
{
	INIT_WORK(&vibrator_work, ImmVibeSPI_Control);
	hrtimer_init(&vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib_timer.function = vibrator_timer_func;
	vib_state = 0; //Default is Vibrator OFF	
	timed_output_dev_register(&timed_dev);
}

MODULE_DESCRIPTION("timed output immersion vibrator device");
MODULE_LICENSE("GPL");
