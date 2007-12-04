/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef MUI_AUDIT_H
#define MUI_AUDIT_H

void AuditDialog(HWND hParent);

// For property sheet Game Audit tab
void InitGameAudit(int gameIndex);
INT_PTR CALLBACK GameAuditDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

int MameUIVerifyRomSet(int game);
int MameUIVerifySampleSet(int game);

const char * GetAuditString(int audit_result);
BOOL IsAuditResultKnown(int audit_result);
BOOL IsAuditResultYes(int audit_result);
BOOL IsAuditResultNo(int audit_result);

#endif
