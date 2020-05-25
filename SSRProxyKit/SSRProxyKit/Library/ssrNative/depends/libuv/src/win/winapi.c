/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>

#include "uv.h"
#include "internal.h"


/* Ntdll function pointers */
sRtlGetVersion pRtlGetVersion;
sRtlNtStatusToDosError pRtlNtStatusToDosError;
sNtDeviceIoControlFile pNtDeviceIoControlFile;
sNtQueryInformationFile pNtQueryInformationFile;
sNtSetInformationFile pNtSetInformationFile;
sNtQueryVolumeInformationFile pNtQueryVolumeInformationFile;
sNtQueryDirectoryFile pNtQueryDirectoryFile;
sNtQuerySystemInformation pNtQuerySystemInformation;

/* Kernel32 function pointers */
sGetQueuedCompletionStatusEx pGetQueuedCompletionStatusEx;

sCancelSynchronousIo pCancelSynchronousIo = NULL;
sCancelIoEx pCancelIoEx = NULL;
sGetFinalPathNameByHandleW pGetFinalPathNameByHandleW = NULL;
sCreateSymbolicLinkW pCreateSymbolicLinkW = NULL;
sReOpenFile pReOpenFile = NULL;
sSetFileCompletionNotificationModes pSetFileCompletionNotificationModes = NULL;
sInitializeConditionVariable pInitializeConditionVariable = NULL;
sWakeConditionVariable pWakeConditionVariable = NULL;
sWakeAllConditionVariable pWakeAllConditionVariable = NULL;
sSleepConditionVariableCS pSleepConditionVariableCS = NULL;

sRegGetValueW pRegGetValueW = NULL;

sConvertInterfaceIndexToLuid pConvertInterfaceIndexToLuid = NULL;
sConvertInterfaceLuidToNameW pConvertInterfaceLuidToNameW = NULL;

/* Powrprof.dll function pointer */
sPowerRegisterSuspendResumeNotification pPowerRegisterSuspendResumeNotification;

/* User32.dll function pointer */
sSetWinEventHook pSetWinEventHook;


void uv_winapi_init(void) {
  HMODULE ntdll_module;
  HMODULE powrprof_module;
  HMODULE user32_module;
  HMODULE kernel32_module;
  HMODULE advapi32_module;
  HMODULE iphlpapi_module;

  ntdll_module = GetModuleHandleA("ntdll.dll");
  if (ntdll_module == NULL) {
    uv_fatal_error(GetLastError(), "GetModuleHandleA");
  }

  pRtlGetVersion = (sRtlGetVersion) GetProcAddress(ntdll_module,
                                                   "RtlGetVersion");

  pRtlNtStatusToDosError = (sRtlNtStatusToDosError) GetProcAddress(
      ntdll_module,
      "RtlNtStatusToDosError");
  if (pRtlNtStatusToDosError == NULL) {
    uv_fatal_error(GetLastError(), "GetProcAddress");
  }

  pNtDeviceIoControlFile = (sNtDeviceIoControlFile) GetProcAddress(
      ntdll_module,
      "NtDeviceIoControlFile");
  if (pNtDeviceIoControlFile == NULL) {
    uv_fatal_error(GetLastError(), "GetProcAddress");
  }

  pNtQueryInformationFile = (sNtQueryInformationFile) GetProcAddress(
      ntdll_module,
      "NtQueryInformationFile");
  if (pNtQueryInformationFile == NULL) {
    uv_fatal_error(GetLastError(), "GetProcAddress");
  }

  pNtSetInformationFile = (sNtSetInformationFile) GetProcAddress(
      ntdll_module,
      "NtSetInformationFile");
  if (pNtSetInformationFile == NULL) {
    uv_fatal_error(GetLastError(), "GetProcAddress");
  }

  pNtQueryVolumeInformationFile = (sNtQueryVolumeInformationFile)
      GetProcAddress(ntdll_module, "NtQueryVolumeInformationFile");
  if (pNtQueryVolumeInformationFile == NULL) {
    uv_fatal_error(GetLastError(), "GetProcAddress");
  }

  pNtQueryDirectoryFile = (sNtQueryDirectoryFile)
      GetProcAddress(ntdll_module, "NtQueryDirectoryFile");
  if (pNtQueryVolumeInformationFile == NULL) {
    uv_fatal_error(GetLastError(), "GetProcAddress");
  }

  pNtQuerySystemInformation = (sNtQuerySystemInformation) GetProcAddress(
      ntdll_module,
      "NtQuerySystemInformation");
  if (pNtQuerySystemInformation == NULL) {
    uv_fatal_error(GetLastError(), "GetProcAddress");
  }

  kernel32_module = GetModuleHandleA("kernel32.dll");
  if (kernel32_module == NULL) {
    uv_fatal_error(GetLastError(), "GetModuleHandleA");
  }

  pGetQueuedCompletionStatusEx = (sGetQueuedCompletionStatusEx) GetProcAddress(
      kernel32_module,
      "GetQueuedCompletionStatusEx");

  pCancelSynchronousIo = (sCancelSynchronousIo) GetProcAddress(kernel32_module, "CancelSynchronousIo");
  pCancelIoEx = (sCancelIoEx) GetProcAddress(kernel32_module, "CancelIoEx");
  pGetFinalPathNameByHandleW = (sGetFinalPathNameByHandleW)GetProcAddress(kernel32_module, "GetFinalPathNameByHandleW");
  pCreateSymbolicLinkW = (sCreateSymbolicLinkW)GetProcAddress(kernel32_module, "CreateSymbolicLinkW");
  pReOpenFile = (sReOpenFile)GetProcAddress(kernel32_module, "ReOpenFile");
  pSetFileCompletionNotificationModes = (sSetFileCompletionNotificationModes)GetProcAddress(kernel32_module, "SetFileCompletionNotificationModes");
  pInitializeConditionVariable = (sInitializeConditionVariable)GetProcAddress(kernel32_module, "InitializeConditionVariable");
  pWakeConditionVariable = (sWakeConditionVariable)GetProcAddress(kernel32_module, "WakeConditionVariable");
  pWakeAllConditionVariable = (sWakeAllConditionVariable)GetProcAddress(kernel32_module, "WakeAllConditionVariable");
  pSleepConditionVariableCS = (sSleepConditionVariableCS)GetProcAddress(kernel32_module, "SleepConditionVariableCS");

  advapi32_module = GetModuleHandleA("advapi32.dll");
  pRegGetValueW = (sRegGetValueW)GetProcAddress(advapi32_module, "RegGetValueW");

  iphlpapi_module = GetModuleHandleA("iphlpapi.dll");
  pConvertInterfaceIndexToLuid = (sConvertInterfaceIndexToLuid) GetProcAddress(iphlpapi_module, "ConvertInterfaceIndexToLuid");
  pConvertInterfaceLuidToNameW = (sConvertInterfaceLuidToNameW) GetProcAddress(iphlpapi_module, "ConvertInterfaceLuidToNameW");

  powrprof_module = LoadLibraryA("powrprof.dll");
  if (powrprof_module != NULL) {
    pPowerRegisterSuspendResumeNotification = (sPowerRegisterSuspendResumeNotification)
      GetProcAddress(powrprof_module, "PowerRegisterSuspendResumeNotification");
  }

  user32_module = LoadLibraryA("user32.dll");
  if (user32_module != NULL) {
    pSetWinEventHook = (sSetWinEventHook)
      GetProcAddress(user32_module, "SetWinEventHook");
  }

}
