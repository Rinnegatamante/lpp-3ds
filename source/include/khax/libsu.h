/*
	Copyright (C) 2015 Jason Dellaluce
	Lib SuperUser for Nintendo 3DS
	
	This lib is meant to gather various exploits of the system
	and use them to offer the user an elevation of his privilegses
	over the console.
	
	Exploits used :
		- Memchunkhax II : An implementation of what derrek, plutoo
		and smea showed here:
		https://media.ccc.de/v/32c3-7240-console_hacking
			realized with the help of Steveice10, julian20, MassExplosion,
			motezazer, TuxSH.
	
	Success rate :
		Still unstable, but with a reliability around 80%.
		The app crashes when returning to Homebrew Launcher or when
		shutting down the process, probably becouse of the corruption of
		kernel objects.
		
	Firmware compatibility:
		ARM11 Kernel:
			Memchunkhax II -> New3DS, Old3DS, 2DS on System Version <= 10.3
*/

// Comment this in order to not show the exploit progress.
#define SUDEBUG

#ifdef __cplusplus
extern "C" {
#endif


/*	
	Library initialization, will gain access to all syscalls, all services,
	and the ARM11 Kernel.
*/
int suInit();



#ifdef __cplusplus
}
#endif

