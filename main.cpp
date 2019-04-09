#include <iostream>
#include <sys/time.h>
#include "sleeping_threads_list.h"


timeval calc_wake_up_timeval(int usecs_to_sleep) {

	timeval now, time_to_sleep, wake_up_timeval;
	gettimeofday(&now, nullptr);
	time_to_sleep.tv_sec = usecs_to_sleep / 1000000;
	time_to_sleep.tv_usec = usecs_to_sleep % 1000000;
	timeradd(&now, &time_to_sleep, &wake_up_timeval);
	return wake_up_timeval;
}


/*
 * In main we are simulating this scenario:
 *
 *  T1	|----- ----- ----- ----- ----- ----- ----- ----- ----- -----|
 *  T2	|-----|
 *  T3	|----- ----- ----- ----- ----- ----- -----|
 *  T4	|----- ---|
 *
*/

int main(int argc, char **argv) {

	timeval t1 = calc_wake_up_timeval(500);
	timeval t2 = calc_wake_up_timeval(50);
	timeval t3 = calc_wake_up_timeval(350);
	timeval t4 = calc_wake_up_timeval(80);

	SleepingThreadsList threads;

	// When adding thread 1 to the list, it is the thread that should wake up first, thus it's the first thread on the list.
	threads.add(1, t1);
	cout << "First thread to wake up is: " << threads.peek()->id
			<< ", its timeval is: " << threads.peek()->awaken_tv.tv_sec << "."
			<< threads.peek()->awaken_tv.tv_usec << endl;

	// T1's timeval is greater than T2's timeval (that means T2 should wake up before T1), so the list will be: T2 -> T1.
	threads.add(2, t2);
	cout << "First thread to wake up is: " << threads.peek()->id
			<< ", its timeval is: " << threads.peek()->awaken_tv.tv_sec << "."
			<< threads.peek()->awaken_tv.tv_usec << endl;

	// When adding T3, the list becomes: T2 -> T3 -> T1
	threads.add(3, t3);
	cout << "First thread to wake up is: " << threads.peek()->id
			<< ", its timeval is: " << threads.peek()->awaken_tv.tv_sec << "."
			<< threads.peek()->awaken_tv.tv_usec << endl;


	// After adding T4, the list becomes: T2 -> T4 -> T3 -> T1
	threads.add(4, t4);
	cout << "First thread to wake up is: " << threads.peek()->id
			<< ", its timeval is: " << threads.peek()->awaken_tv.tv_sec << "."
			<< threads.peek()->awaken_tv.tv_usec << endl;

	// When it's time to wake up T2, we can remove it from our list, that becomes: T4 -> T3 -> T1
	threads.pop();
	cout << "First thread to wake up is: " << threads.peek()->id
			<< ", its timeval is: " << threads.peek()->awaken_tv.tv_sec << "."
			<< threads.peek()->awaken_tv.tv_usec << endl;


	return 0;
}
