/*
* XpilotStudio, the XPilot Map Editor for Windows 95/98/NT.  Copyright (C) 2000 by
*
*      Jarrod L. Miller           <jlmiller@ctitech.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
* See the file COPYRIGHT.TXT for current copyright information.
*
*/

#include "win_xpstudio.h"

static POINT ptBeg, ptEnd;
static POINT ptCurse;
static int cxClient, cyClient;
/***************************************************************************/
/* CreateMapEditor                                                         */
/* Arguments :                                                             */
/*                                                                         */
/* Purpose :   Creates a new map editor window                             */
/***************************************************************************/
HWND CreateMapEditor(HWND hwndLocClient){
    MDICREATESTRUCT mdicreate;
	HWND hwndNewMap = NULL;

	mdicreate.szClass = "MapEditor";
	mdicreate.szTitle = "New Map";
	mdicreate.hOwner	= hInst;
	mdicreate.x = CW_USEDEFAULT;
	mdicreate.cx = CW_USEDEFAULT;
	mdicreate.y = CW_USEDEFAULT;
	mdicreate.cy = CW_USEDEFAULT;
	mdicreate.style = WS_VSCROLL | WS_HSCROLL;
	mdicreate.lParam = 0;
	
	hwndNewMap = (HWND)SendMessage (hwndLocClient,
		WM_MDICREATE,
		0,
		(LONG)(LPMDICREATESTRUCT)&mdicreate);

	return hwndNewMap;
}
/***************************************************************************/
/* MapEditorWndProc                                                        */
/* Arguments :                                                             */
/*    hwnd                                                                 */
/*    iMsg                                                                 */
/*    wParam                                                               */
/*    lParam                                                               */
/*                                                                         */
/* Purpose :   The procedure for the child map windows.                    */
/***************************************************************************/
LRESULT CALLBACK MapEditorWndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndLocClient, hwndLocFrame;
	LPXPSTUDIODOCUMENT lpXpStudioDocument;
	LPMAPDOCUMENT lpMapDocument ;
	PAINTSTRUCT ps ;
	HDC         hdc ;
//	RECT rect;
	int iVscrollInc, iHscrollInc, x, y;
	static int dx, dy;
	char sbtext[100];

	switch (iMsg)
	{
	case WM_CREATE:
		lpXpStudioDocument = CreateNewXpDocument(MAPFILE);
		lpXpStudioDocument->lpMapDocument = CreateNewMapDoc();
		SetWindowLong(hwnd, GWL_USERDATA, (long) lpXpStudioDocument);
		hwndActive = hwnd;
		hwndLocClient = GetParent(hwnd);
		hwndLocFrame = GetParent (hwndLocClient);
		return 0;
	
	case WM_COMMAND :
		lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
		lpMapDocument = lpXpStudioDocument->lpMapDocument;
		switch (LOWORD (wParam))
		{
		case IDM_PROPERTIES :
			CreatePropertySheet(hwnd);
			break;
		case IDM_ZOOMIN :
			if (lpMapDocument->view_zoom < 100)
			{
				lpMapDocument->view_zoom += 5;
				InvalidateRect (hwnd, NULL, TRUE);
			}
			break;			
		case IDM_ZOOMOUT :
			if (lpMapDocument->view_zoom <= 6)
				lpMapDocument->view_zoom = 2;
			else
				lpMapDocument->view_zoom -= 5;
			InvalidateRect (hwnd, NULL, TRUE);
			break;
		case IDM_DELETEITEM :
			if (lpMapDocument->selectedpoly || lpMapDocument->selecteditem)
				DeleteMapItem(lpMapDocument);
			break;
		case IDM_REFRESH :
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		case IDM_UPDATE_ITEM_PARAMS:
			GetSetEnterableInfo();
			UpdateSelected(lpMapDocument);
			break;
		}
		//Right now we'll always repaint the entire window after a command.
		InvalidateRect (hwnd, NULL, TRUE);
		return 0;
		
	case WM_KEYDOWN :
		switch (wParam)
		{
		case VK_HOME :
			SendMessage (hwnd, WM_VSCROLL, SB_TOP, 0L);
			break ;
		case VK_END :
			SendMessage (hwnd, WM_VSCROLL, SB_BOTTOM, 0L);
			break ;
		case VK_PRIOR :
			SendMessage (hwnd, WM_VSCROLL, SB_PAGEUP, 0L);
			break ;
		case VK_NEXT :
			SendMessage (hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
			break ;			
		case VK_UP :
			SendMessage (hwnd, WM_VSCROLL, SB_LINEUP, 0L);
			break ;			
		case VK_DOWN :
			SendMessage (hwnd, WM_VSCROLL, SB_LINEDOWN, 0L);
			break ;			
		case VK_LEFT :
			SendMessage (hwnd, WM_HSCROLL, SB_LINEUP, 0L);
			break ;			
		case VK_RIGHT :
			SendMessage (hwnd, WM_HSCROLL, SB_LINEDOWN, 0L);
			break ;
		}
		return 0;
	case WM_VSCROLL :
		lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
		lpMapDocument = lpXpStudioDocument->lpMapDocument;
		switch (LOWORD (wParam))
		{
		case SB_LINEUP :
			iVscrollInc = -35 ;
			break ;
			
		case SB_LINEDOWN :
			iVscrollInc = 35 ;
			break ;
			
		default :
			iVscrollInc = 0 ;
		}
		
		if (iVscrollInc != 0)
		{
			lpMapDocument->view_y += iVscrollInc ;
			while (lpMapDocument->view_y < 0)
				lpMapDocument->view_y+= lpMapDocument->height;
			while (lpMapDocument->view_y >= lpMapDocument->height)
				lpMapDocument->view_y-= lpMapDocument->height;
			InvalidateRect(hwnd, NULL, TRUE);
//			ScrollWindow(hwnd, 0, -iVscrollInc, NULL, NULL) ;
//			UpdateWindow(hwnd);
			sprintf(sbtext, "view_x:%d view_y:%d zoom: %f", lpMapDocument->view_x,lpMapDocument->view_y, lpMapDocument->view_zoom);
			SendMessage(hwndStatusBar, SB_SETTEXT, 0, (LPARAM) (LPSTR) sbtext);
			ptBeg.y -= iVscrollInc;
		}
		return 0 ;
	case WM_HSCROLL :
		lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
		lpMapDocument = lpXpStudioDocument->lpMapDocument;
		switch (LOWORD (wParam))
		{
		case SB_LINEUP :
			iHscrollInc = -35;
			break;
			
		case SB_LINEDOWN :
			iHscrollInc = 35;
			break;
			
		default :
			iHscrollInc = 0;
		}
		
		if (iHscrollInc != 0)
		{
			lpMapDocument->view_x += iHscrollInc;
			while (lpMapDocument->view_x < 0)
				lpMapDocument->view_x+= lpMapDocument->width;
			while (lpMapDocument->view_x >= lpMapDocument->width)
				lpMapDocument->view_x-= lpMapDocument->width;
			InvalidateRect(hwnd, NULL, TRUE);
	
//			ScrollWindow(hwnd, -iHscrollInc, 0, NULL, NULL) ;
//			UpdateWindow(hwnd);
			sprintf(sbtext, "view_x:%d view_y:%d zoom: %f", lpMapDocument->view_x,lpMapDocument->view_y, lpMapDocument->view_zoom);
			SendMessage(hwndStatusBar, SB_SETTEXT, 0, (LPARAM) (LPSTR) sbtext);
			ptBeg.x -= iHscrollInc;
		}
		return 0 ;

	case WM_PAINT :
		hdc = BeginPaint( hwnd, &ps) ;
		{
			lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
			lpMapDocument = lpXpStudioDocument->lpMapDocument;

			hwndTemp = hwnd;
//			SetScrollRange (hwnd, SB_VERT, 0, 1, FALSE) ;

			lpMapDocument->view_zoom =  ((float) (ps.rcPaint.right - ps.rcPaint.left) / (float) lpMapDocument->width);
			
/*			Rectangle(hdc, lpMapDocument->view_x,
				lpMapDocument->view_y,
				lpMapDocument->view_x-lpMapDocument->width,
				lpMapDocument->view_y-lpMapDocument->height);*/

			//Hopefully temporary hack to draw the entire map.
			//This actually draws the map 9 times...which is really
			//inefficient. We need some way to only draw what
			//we're currently seeing.
			DrawMapEntire(lpMapDocument);
		}	
		EndPaint (hwnd, &ps) ;
		return 0 ;

	case WM_LBUTTONDOWN:
		lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
		lpMapDocument = lpXpStudioDocument->lpMapDocument;

//		SetCapture(hwnd);
		GetCursorPos(&ptCurse);
		ScreenToClient (hwnd, &ptCurse);
		x = (ptCurse.x+lpMapDocument->view_x);
		y = (ptCurse.y+lpMapDocument->view_y);
		
		switch (iSelectionMapTools)
		{
		case IDM_PEN :
			fDrawing = TRUE;
			if(!CreateItem(lpMapDocument, WRAP_XPIXEL(x), lpMapDocument->height-WRAP_YPIXEL(y), dx, dy, iSelectionMapSyms, FALSE))
			{
			ptBeg.x = ptEnd.x = LOWORD (lParam);
			ptBeg.y = ptEnd.y = HIWORD (lParam);
			InvalidateRect(hwnd, NULL, TRUE);
			DrawHighlightLine (hwnd, ptBeg, ptEnd);
			dx = dy = 0;
			}
			break;
		case IDM_MODIFYITEM :
			DoModifyCommand(lpMapDocument, WRAP_XPIXEL(x), lpMapDocument->height-WRAP_YPIXEL(y), iSelectionMapModify);
			SendMessage(hwndLocFrame, WM_COMMAND, (WPARAM) UPDATE_COMMANDS, 0);
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		}

		if (fDragging)
		{
			ptBeg.x = ptEnd.x = LOWORD (lParam);
			ptBeg.y = ptEnd.y = HIWORD (lParam);
			DrawHighlightLine (hwnd, ptBeg, ptEnd);
		}
		return 0;
		
	case WM_MOUSEMOVE:
		/*Verify that we should be processing this message. If this isnt the active window, then
		we have no need to process this message for this window*/
/*		hwndActive = (HWND) SendMessage (hwndLocClient, WM_MDIGETACTIVE, 0, 0);
		if (hwndActive != hwnd)
			return 0;
*/
		lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
		lpMapDocument = lpXpStudioDocument->lpMapDocument;
		GetCursorPos(&ptCurse);
		ScreenToClient (hwnd, &ptCurse);
		x = ptCurse.x + lpMapDocument->view_x;
		y = ptCurse.y + lpMapDocument->view_y;

		sprintf(sbtext, "X:%d Y:%d", WRAP_XPIXEL(x), lpMapDocument->height-WRAP_YPIXEL(y));
		SendMessage(hwndStatusBar, SB_SETTEXT, 0, (LPARAM) (LPSTR) sbtext);

		if (fDrawing || fDragging)
		{
//			switch (iSelectionMapTools)
//			{
//			case IDM_PEN :
				SetCursor (LoadCursor (NULL, IDC_CROSS));
				DrawHighlightLine (hwnd, ptBeg, ptEnd);
				ptEnd.x = (x - lpMapDocument->view_x);
				ptEnd.y = (y - lpMapDocument->view_y);
				dx = ptEnd.x - ptBeg.x;
				dy = -(ptEnd.y - ptBeg.y);
				DrawHighlightLine (hwnd, ptBeg, ptEnd);
//				break;
//			}
		}
		else
			switch (iSelectionMapTools)
			{
				case IDM_MODIFYITEM:
				switch (iSelectionMapModify)
				{
				case IDM_ADDVERTEX:
					if (lpMapDocument->selectedpoly)
					{
						SetCursor (LoadCursor (NULL, IDC_CROSS));
						DrawHighlightLine (hwnd, ptBeg, ptEnd);
						ptBeg.x = lpMapDocument->selectedpoly->vertex[lpMapDocument->selectedpoly->num_verts-1].x - lpMapDocument->view_x;
						ptBeg.y = lpMapDocument->height - lpMapDocument->selectedpoly->vertex[lpMapDocument->selectedpoly->num_verts-1].y - lpMapDocument->view_y;
//						DrawHighlightLine (hwnd, ptBeg, ptEnd);
						ptEnd.x = (x - lpMapDocument->view_x);
						ptEnd.y = (y - lpMapDocument->view_y);
						DrawHighlightLine (hwnd, ptBeg, ptEnd);
					}
					break;

			case IDM_MOVEVERTEX:
			case IDM_DELVERTEX:
					if (lpMapDocument->selectedpoly)
					{
						lpMapDocument->numselvert = FindClosestVertex(lpMapDocument, lpMapDocument->selectedpoly, x,  lpMapDocument->height-y);
						InvalidateRect(hwnd, NULL, TRUE);
						//DrawVertexList(lpMapDocument, lpMapDocument->selectedpoly, IDM_MAP_WALL, lpMapDocument->view_x, lpMapDocument->view_y);
					}
					break;
				}
				break;
			}


		return 0;

	case WM_RBUTTONDOWN:
		lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
		lpMapDocument = lpXpStudioDocument->lpMapDocument;
		switch (iSelectionMapTools)
		{
		case IDM_PEN :
			if (!CreateItem(lpMapDocument, 0, 0, 0, 0, iSelectionMapSyms, TRUE))
			{
				fDrawing = FALSE;
				InvalidateRect(hwnd, NULL, TRUE);
				SetCursor (LoadCursor (NULL, IDC_ARROW));
			DrawHighlightLine (hwnd, ptBeg, ptEnd);
			ptBeg.x = ptBeg.y = ptEnd.x = ptEnd.y = 0;
			}
			break;
		}
		switch (iSelectionMapModify)
		{
		case IDM_ADDVERTEX :
			if (!CreateItem(lpMapDocument, 0, 0, 0, 0, iSelectionMapSyms, TRUE))
			{
				SendMessage(hwndMapModifyToolBar, TB_CHECKBUTTON, IDM_PICKITEM, TRUE); 
				SendMessage(hwndLocFrame, WM_COMMAND, IDM_PICKITEM, TRUE);
				lpMapDocument->selectedpoly->selected = FALSE;
				lpMapDocument->selectedpoly = NULL;
			}
			fDrawing = FALSE;
//			fCreatingPolygon = FALSE;
			InvalidateRect(hwnd, NULL, TRUE);
			SetCursor (LoadCursor (NULL, IDC_ARROW));
			DrawHighlightLine (hwnd, ptBeg, ptEnd);
			ptBeg.x = ptBeg.y = ptEnd.x = ptEnd.y = 0;
			break;
		}
		return 0;

	case WM_MDIACTIVATE:
		if(lParam == (LPARAM) hwnd)
		{
			lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
			lpMapDocument = lpXpStudioDocument->lpMapDocument;
			hwndActive = hwnd;
			SendMessage(hwndLocFrame, WM_COMMAND, (WPARAM) UPDATE_COMMANDS, 0);
			lpMapDocument->selectedbool = FALSE;
//			DoCaption(lpMapDocument, hwnd, lpMapDocument->MapStruct.mapName);
		}
//		else
//			hwndActive = NULL;
		return 0;
		
	case WM_DESTROY:
		lpXpStudioDocument = (LPXPSTUDIODOCUMENT) GetWindowLong (hwnd, GWL_USERDATA);
		lpMapDocument = lpXpStudioDocument->lpMapDocument;
		free(lpMapDocument);
		free(lpXpStudioDocument);
		hwndActive = NULL;
		SendMessage(hwndLocFrame, WM_COMMAND, (WPARAM) UPDATE_COMMANDS, 0);
		return 0;
	}
	//Pass unprocessed message to DefMDIChildProc
    return DefMDIChildProc (hwnd, iMsg, wParam, lParam);
}
