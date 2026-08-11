/*!
\file calculator.h
\brief Public entry point for a calculator client thread.
*/

#ifndef CALCULATOR_H
#define CALCULATOR_H

/*!
Connects to the monitor, receives a range of integers to sum, and
reports progress back until the range is exhausted or this thread is
cancelled (simulating a crashed calculator).
\param arg a calculator_param*, see structures.h
*/
void *CreateTCPClientSocket(void *arg);

#endif /* CALCULATOR_H */
