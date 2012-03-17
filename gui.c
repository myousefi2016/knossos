/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2012
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

/*
 *  This file contains the code for the agar gui and associated
 *  functions.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <SDL/SDL.h>
#include <SDL/SDL_Clipboard.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <agar/core.h>
#include <agar/gui.h>

#include "knossos-global.h"
#include "gui.h"
#include "customStyle.h"

extern struct stateInfo *tempConfig;
extern struct stateInfo *state;
AG_Style customAgarStyle;

AG_Style customAgarStyle;

int32_t initGUI() {
    SDL_Surface *icon;
    uint32_t iconColorKey;

    /* initialize the agar core & video system */
	if(AG_InitCore("KNOSSOS", 0) == -1
       || AG_InitVideo(state->viewerState->screenSizeX, state->viewerState->screenSizeY, 32, AG_VIDEO_OPENGL|AG_VIDEO_RESIZABLE)
       // || AG_InitGraphics("sdlgl") seems to be the same as ag_init_video..
       == -1) {
	    return (1);
	}

    /* Initialize our custom Agar theme */
    InitCustomAgarStyle(&customAgarStyle);
    AG_SetStyle(agView, &customAgarStyle);

    /* set the window caption */
    updateTitlebar(FALSE);

    if((icon = SDL_LoadBMP("icon"))) {
        iconColorKey = SDL_MapRGB(icon->format, 255, 255, 255);
        SDL_SetColorKey(icon, SDL_SRCCOLORKEY, iconColorKey);
        SDL_WM_SetIcon(icon, NULL);
    }

    if(SDL_EnableKeyRepeat(200, (1000 / state->viewerState->stepsPerSec)) == -1) {
        LOG("Error setting key repeat parameters.");
    }

    AG_SetVideoResizeCallback(resizeCallback);

    /* display some basic openGL driver statistics */
    printf("OpenGL v%s on %s from %s\n", glGetString(GL_VERSION),
        glGetString(GL_RENDERER), glGetString(GL_VENDOR));

    /* printf("%s\n", glGetString(GL_EXTENSIONS)); */

    AG_ColorsSetRGB(BG_COLOR, 128, 128, 128);
    setGUIcolors();

    state->viewerState->ag->oneShiftedCurrPos.x =
        state->viewerState->currentPosition.x + 1;
    state->viewerState->ag->oneShiftedCurrPos.y =
        state->viewerState->currentPosition.y + 1;
    state->viewerState->ag->oneShiftedCurrPos.z =
        state->viewerState->currentPosition.z + 1;

    state->viewerState->ag->activeTreeID = 1;
    state->viewerState->ag->activeNodeID = 1;

    state->viewerState->ag->totalNodes = 0;
    state->viewerState->ag->totalTrees = 0;


    state->viewerState->ag->radioSkeletonDisplayMode = 0;

    /* init here instead of initSkeletonizer to fix some init order issue */
    state->skeletonState->displayMode = 0;
    state->skeletonState->displayMode |= DSP_SKEL_VP_WHOLE;

    state->viewerState->ag->radioRenderingModel = 1;

    state->viewerState->ag->commentBuffer = malloc(10240 * sizeof(char));
    memset(state->viewerState->ag->commentBuffer, '\0', 10240 * sizeof(char));

    state->viewerState->ag->commentSearchBuffer = malloc(2048 * sizeof(char));
    memset(state->viewerState->ag->commentSearchBuffer, '\0', 2048 * sizeof(char));

    createMenuBar(state);
    createCoordBarWin(state);
    createSkeletonVpToolsWin(state);
    createDataSizeWin(state);
    createNavWin(state);
    createToolsWin();
    createConsoleWin(state);

    createNavOptionsWin(state);
    createSyncOptionsWin(state);
    createSaveOptionsWin(state);

    createAboutWin();
    /* all following 4 unused
    createDisplayOptionsWin(state);
    createRenderingOptionsWin(state);
    createSpatialLockingOptionsWin(state);
    createVolTracingOptionsWin(state);   */

    createDataSetStatsWin(state);
    createViewPortPrefWin();
    createZoomingWin(state);
    /*createSetDynRangeWin(state); */           /* Unused. */

    createVpXzWin(state);
    createVpXyWin(state);
    createVpYzWin(state);
    createVpSkelWin(state);

    UI_loadSettings();

    return TRUE;
}


void updateAGconfig() {
    state->viewerState->ag->totalTrees = state->skeletonState->treeElements;
    state->viewerState->ag->totalNodes = state->skeletonState->totalNodeElements;
    if(state->skeletonState->totalNodeElements == 0) {
        AG_NumericalSetWriteable(state->viewerState->ag->actNodeIDWdgt1, FALSE);
        AG_NumericalSetWriteable(state->viewerState->ag->actNodeIDWdgt2, FALSE);
        state->viewerState->ag->activeNodeID = 0;
        state->viewerState->ag->activeNodeCoord.x = 0;
        state->viewerState->ag->activeNodeCoord.y = 0;
        state->viewerState->ag->activeNodeCoord.z = 0;
    }
    else {
        AG_NumericalSetWriteable(state->viewerState->ag->actNodeIDWdgt1, TRUE);
        AG_NumericalSetWriteable(state->viewerState->ag->actNodeIDWdgt2, TRUE);
    }

    if(state->skeletonState->activeNode) {
        SET_COORDINATE(state->viewerState->ag->activeNodeCoord,
            state->skeletonState->activeNode->position.x + 1,
            state->skeletonState->activeNode->position.y + 1,
            state->skeletonState->activeNode->position.z + 1)
        state->viewerState->ag->actNodeRadius =
            state->skeletonState->activeNode->radius;
    }

    if(state->skeletonState->activeTree) {
        state->viewerState->ag->actTreeColor =
            state->skeletonState->activeTree->color;
    }



    SET_COORDINATE(state->viewerState->ag->oneShiftedCurrPos,
        state->viewerState->currentPosition.x + 1,
        state->viewerState->currentPosition.y + 1,
        state->viewerState->currentPosition.z + 1)

    state->viewerState->ag->numBranchPoints =
        state->skeletonState->branchStack->elementsOnStack;

    strncpy(state->viewerState->ag->commentBuffer,
        state->skeletonState->commentBuffer,
        10240);
}


/* functions to create ui components  */

static void yesNoPromptHitYes(AG_Event *event) {
    AG_Window *prompt = AG_PTR(1);
    void (*cb)() = AG_PTR(2);
    AG_Window *par = AG_PTR(3);

    if(cb != NULL)
        (*cb)(par);

    AG_ObjectDetach(prompt);
}

static void yesNoPromptHitNo(AG_Event *event) {
    AG_Window *prompt = AG_PTR(1);
    void (*cb)() = AG_PTR(2);
    AG_Window *par = AG_PTR(3);

    if(cb != NULL)
        (*cb)(par);

    AG_ObjectDetach(prompt);
}

void createMenuBar(struct stateInfo *state) {
	AG_Menu *appMenu;
	AG_MenuItem *menuItem;
	AG_MenuItem *subMenuItem;

    appMenu = AG_MenuNewGlobal(0);
    state->viewerState->ag->appMenuRoot = appMenu->root;

	menuItem = AG_MenuNode(appMenu->root, "File", NULL);
	{
		AG_MenuAction(menuItem, "Open...", NULL, fileOpenSkelFile, "%p", state);
        AG_MenuNode(menuItem, "Recent Files", NULL);
		AG_MenuAction(menuItem, "Save (CTRL+s)", NULL, UI_saveSkeleton, "%d", TRUE);
		AG_MenuAction(menuItem, "Save As...", NULL, fileSaveAsSkelFile, "%p", state);
        AG_MenuSeparator(menuItem);
		AG_MenuAction(menuItem, "Quit", agIconClose.s, UI_checkQuitKnossos, NULL);
	}

    menuItem = AG_MenuNode(appMenu->root, "Edit Skeleton", NULL);
	{
        subMenuItem = AG_MenuNode(menuItem, "Work Mode", NULL);
            AG_MenuAction(subMenuItem, "Add Node (a)", NULL, UI_workModeAdd, NULL);
            AG_MenuAction(subMenuItem, "Link with Active Node (w)", NULL, UI_workModeLink, NULL);
            AG_MenuAction(subMenuItem, "Drop Nodes", NULL, UI_workModeDrop, NULL);
		AG_MenuAction(menuItem, "Skeleton Statistics...", NULL, UI_unimplemented, NULL);
		AG_MenuAction(menuItem, "Clear Skeleton", NULL, UI_clearSkeleton, NULL);
	}
    menuItem = AG_MenuNode(appMenu->root, "View", NULL);
	{
        subMenuItem = AG_MenuNode(menuItem, "Work Mode", NULL);
            AG_MenuAction(subMenuItem, "Drag Dataset", NULL, UI_setViewModeDrag, NULL);
            AG_MenuAction(subMenuItem, "Recenter on Click", NULL, UI_setViewModeRecenter, NULL);
		AG_MenuAction(menuItem, "Dataset Statistics", NULL,  UI_unimplemented, NULL, NULL);
        AG_MenuAction(menuItem, "Zoom...", NULL, viewZooming, NULL, NULL);
	}

    menuItem = AG_MenuNode(appMenu->root, "Preferences", NULL);
	{
        AG_MenuAction(menuItem, "Load Custom Preferences", NULL, prefLoadCustomPrefs, NULL, NULL);
        AG_MenuAction(menuItem, "Save Custom Preferences As", NULL, prefSaveCustomPrefsAs, NULL, NULL);
        AG_MenuAction(menuItem, "Dataset Navigation", NULL, prefNavOptions, NULL);
        AG_MenuAction(menuItem, "Synchronization", NULL, prefSyncOptions, NULL);
		AG_MenuAction(menuItem, "Data Saving Options", NULL, prefSaveOptions, NULL);
        AG_MenuAction(menuItem, "Viewport Settings", NULL, prefViewportPrefs, NULL);
	}

    menuItem = AG_MenuNode(appMenu->root, "Windows", NULL);
	{
		//AG_MenuAction(menuItem, "navigation", NULL, winShowNavigator, "%p", state);
		AG_MenuAction(menuItem, "Tools", NULL, winShowTools, "%p", state);
		AG_MenuAction(menuItem, "Log", NULL, winShowConsole, "%p", state);
	}

    menuItem = AG_MenuNode(appMenu->root, "Help", NULL);
	{
		//AG_MenuAction(menuItem, "Open PDF help", NULL, NULL, NULL);
		AG_MenuAction(menuItem, "About", NULL, UI_helpShowAbout, NULL);
	}

}

void createNavOptionsWin(struct stateInfo *state) {
    AG_Numerical *numerical;

	state->viewerState->ag->navOptWin = AG_WindowNew(0);

    AG_WindowSetSideBorders(state->viewerState->ag->navOptWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->navOptWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->navOptWin, "Navigation Settings");
    AG_WindowSetGeometryAligned(state->viewerState->ag->navOptWin, AG_WINDOW_MC, 200, 95);

    numerical = AG_NumericalNewUint(state->viewerState->ag->navOptWin, 0, NULL, "Movement Speed: ", &tempConfig->viewerState->stepsPerSec);
    {
        AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
        AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
    }

    numerical = AG_NumericalNewUint(state->viewerState->ag->navOptWin, 0, NULL, "Jump Frames: ", &tempConfig->viewerState->dropFrames);
    {
        AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
        AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
    }

    numerical = AG_NumericalNewUint(state->viewerState->ag->navOptWin, 0, NULL, "Recentering Time [ms]: ", &tempConfig->viewerState->recenteringTime);
    {
        AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
        AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
    }

    AG_WindowSetCloseAction(state->viewerState->ag->navOptWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->navOptWin);
}

void createSpatialLockingOptionsWin(struct stateInfo *state) {
	state->viewerState->ag->spatLockOptWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->spatLockOptWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->spatLockOptWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->spatLockOptWin, "Spatial Locking");
    AG_WindowSetGeometryAligned(state->viewerState->ag->spatLockOptWin, AG_WINDOW_MC, 200, 120);

    AG_WindowSetCloseAction(state->viewerState->ag->spatLockOptWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->spatLockOptWin);
}

void createVolTracingOptionsWin(struct stateInfo *state) {
	state->viewerState->ag->volTraceOptWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->volTraceOptWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->volTraceOptWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->volTraceOptWin, "Volumetric Tracing");
    AG_WindowSetGeometryAligned(state->viewerState->ag->volTraceOptWin, AG_WINDOW_MC, 210, 125);

    AG_WindowSetCloseAction(state->viewerState->ag->volTraceOptWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->volTraceOptWin);
}


void createRenderingOptionsWin(struct stateInfo *state) {
	state->viewerState->ag->renderingOptWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->renderingOptWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->renderingOptWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->renderingOptWin, "Rendering Quality");
    AG_WindowSetGeometryAligned(state->viewerState->ag->renderingOptWin, AG_WINDOW_MC, 200, 65);
    AG_CheckboxNew(state->viewerState->ag->renderingOptWin, 0, "Light Effects");


    AG_WindowSetCloseAction(state->viewerState->ag->renderingOptWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->renderingOptWin);
}

void createDisplayOptionsWin(struct stateInfo *state) {

	state->viewerState->ag->dispOptWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->dispOptWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->dispOptWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->dispOptWin, "Display Settings");
    AG_WindowSetGeometryAligned(state->viewerState->ag->dispOptWin, AG_WINDOW_MC, 300, 250);
    AG_WindowSetCloseAction(state->viewerState->ag->dispOptWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->dispOptWin);
}

void createSyncOptionsWin(struct stateInfo *state) {
    AG_Button *button;
    AG_Textbox *textbox;
    AG_Box *box;
    AG_Numerical *numerical;

	state->viewerState->ag->syncOptWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->syncOptWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->syncOptWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->syncOptWin, "Synchronization Settings");
    AG_WindowSetGeometryAligned(state->viewerState->ag->syncOptWin, AG_WINDOW_MC, 200, 120);

    state->viewerState->ag->syncOptLabel = AG_LabelNew(state->viewerState->ag->syncOptWin, 0, NULL);
    {
        AG_ExpandHoriz(state->viewerState->ag->syncOptLabel);
    }

    textbox = AG_TextboxNew(state->viewerState->ag->syncOptWin, AG_TEXTBOX_INT_ONLY|AG_TEXTBOX_ABANDON_FOCUS, "Remote Host: ");
    {
        AG_TextboxBindASCII(textbox, tempConfig->clientState->serverAddress, 1024);
        AG_SetEvent(textbox, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
        AG_SetEvent(textbox, "widget-lostfocus", agInputWdgtLostFocus, NULL);
    }

    numerical = AG_NumericalNewIntR(state->viewerState->ag->syncOptWin, 0, NULL, "Remote Port: ", &tempConfig->clientState->remotePort, 1, 65536);
    {
        AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
        AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
    }

    box = AG_BoxNew(state->viewerState->ag->syncOptWin, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
    {
        AG_ExpandHoriz(box);
    }
    button = AG_ButtonNewFn(box, 0, "Connect", UI_SyncConnect, NULL);
    {
        AG_ExpandHoriz(button);
    }
    button = AG_ButtonNewFn(box, 0, "Disconnect", UI_SyncDisconnect, NULL);
    {
        AG_ExpandHoriz(button);
    }

    AG_WindowSetCloseAction(state->viewerState->ag->syncOptWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->syncOptWin);
}

void createSaveOptionsWin(struct stateInfo *state) {
    AG_Window *win;
    AG_Numerical *numerical;

	win = AG_WindowNew(0);
    {
        AG_WindowSetSideBorders(win, 3);
        AG_WindowSetBottomBorder(win, 3);
        AG_WindowSetCaption(win, "Data Saving Options");
        AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, 200, 80);

        AG_CheckboxNewInt(win, 0, "Auto-Saving", &state->skeletonState->autoSaveBool);

        numerical = AG_NumericalNewUint(win, 0, NULL, "Saving Interval [min]: ", &state->skeletonState->autoSaveInterval);
        {
           AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
           AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }

        AG_CheckboxNewInt(win, 0, "Auto Increment Filename", &state->skeletonState->autoFilenameIncrementBool);

        AG_WindowSetCloseAction(win, AG_WINDOW_HIDE);
    	AG_WindowHide(win);
    }

    state->viewerState->ag->saveOptWin = win;
}

void createCoordBarWin(struct stateInfo *state) {
	state->viewerState->ag->coordBarWin = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_NOBACKGROUND);
	/* if you change that: set coord bar win stuff also for the resize event! should be a define */
    AG_WindowSetGeometryAligned(state->viewerState->ag->coordBarWin, AG_WINDOW_TR, 500, 20);
    AG_WindowSetPadding(state->viewerState->ag->coordBarWin, 0, 0, 3, 1);
    createCurrPosWdgt(state->viewerState->ag->coordBarWin);
	AG_WindowShow(state->viewerState->ag->coordBarWin);
}

void createSkeletonVpToolsWin(struct stateInfo *state) {
    AG_Window *win;

    win = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_NOBACKGROUND);
    AG_WindowSetGeometryBounded(win,
                                state->viewerState->viewPorts[VIEWPORT_SKELETON].upperLeftCorner.x + state->viewerState->viewPorts[VIEWPORT_SKELETON].edgeLength - 160,
                                state->viewerState->viewPorts[VIEWPORT_SKELETON].upperLeftCorner.y + 5,
                                150,
                                20);
    AG_WindowSetPadding(win, 0, 0, 3, 1);
    createSkeletonVpToolsWdgt(win);
    AG_WindowShow(win);

    state->viewerState->ag->skeletonVpToolsWin = win;
}

void reloadDataSizeWin(struct stateInfo *state) {
    float heightxy = state->viewerState->viewPorts[0].displayedlengthInNmY*0.001;
    float widthxy = state->viewerState->viewPorts[0].displayedlengthInNmX*0.001;
    float heightxz = state->viewerState->viewPorts[1].displayedlengthInNmY*0.001;
    float widthxz = state->viewerState->viewPorts[1].displayedlengthInNmX*0.001;
    float heightyz = state->viewerState->viewPorts[2].displayedlengthInNmY*0.001;
    float widthyz = state->viewerState->viewPorts[2].displayedlengthInNmX*0.001;

    if ((heightxy > 1.0) && (widthxy > 1.0)){
        AG_LabelText(state->viewerState->ag->dataSizeLabelxy, "Height %.2f \u00B5m, Width %.2f \u00B5m", heightxy, widthxy);
    }
    else{
        AG_LabelText(state->viewerState->ag->dataSizeLabelxy, "Height %.0f nm, Width %.0f nm", heightxy*1000, widthxy*1000);
    }
    if ((heightxz > 1.0) && (widthxz > 1.0)){
        AG_LabelText(state->viewerState->ag->dataSizeLabelxz, "Height %.2f \u00B5m, Width %.2f \u00B5m", heightxz, widthxz);
    }
    else{
        AG_LabelText(state->viewerState->ag->dataSizeLabelxz, "Height %.0f nm, Width %.0f nm", heightxz*1000, widthxz*1000);
    }

    if ((heightyz > 1.0) && (widthyz > 1.0)){
        AG_LabelText(state->viewerState->ag->dataSizeLabelyz, "Height %.2f \u00B5m, Width %.2f \u00B5m", heightyz, widthyz);
    }
    else{
        AG_LabelText(state->viewerState->ag->dataSizeLabelyz, "Height %.0f nm, Width %.0f nm", heightyz*1000, widthyz*1000);
    }
}

void createDataSizeWin(struct stateInfo *state) {
    AG_Window *win;
    AG_Label *label;

    win = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_NOBACKGROUND);
    AG_WindowSetPadding(win, 0, 0, 3, 1);
    label = AG_LabelNew(win, 0, " ");
    {
        AG_LabelSetPadding(label, 1, 1, 1, 1);
        AG_ExpandHoriz(label);
        state->viewerState->ag->dataSizeLabelxy = label;
    }
    AG_WindowSetGeometryBounded(win,
                                state->viewerState->viewPorts[VIEWPORT_XY].upperLeftCorner.x + 5,
                                state->viewerState->viewPorts[VIEWPORT_XY].upperLeftCorner.y + 5,
                                200,
                                20);
    AG_WindowShow(win);
    state->viewerState->ag->dataSizeWinxy = win;

    win = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_NOBACKGROUND);
    AG_WindowSetPadding(win, 0, 0, 3, 1);
    label = AG_LabelNew(win,0, " ");
    {
        AG_LabelSetPadding(label, 1, 1, 1, 1);
        AG_ExpandHoriz(label);
        state->viewerState->ag->dataSizeLabelxz = label;
    }
    AG_WindowSetGeometryBounded(win,
                                state->viewerState->viewPorts[VIEWPORT_XZ].upperLeftCorner.x + 5,
                                state->viewerState->viewPorts[VIEWPORT_XZ].upperLeftCorner.y + 5,
                                200,
                                20);
    AG_WindowShow(win);
    state->viewerState->ag->dataSizeWinxz = win;

    win = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_NOBACKGROUND);
    AG_WindowSetPadding(win, 0, 0, 3, 1);
    label = AG_LabelNew(win,0, " ");
    {
        AG_LabelSetPadding(label, 1, 1, 1, 1);
        AG_ExpandHoriz(label);
        state->viewerState->ag->dataSizeLabelyz = label;
    }
    AG_WindowSetGeometryBounded(win,
                                state->viewerState->viewPorts[VIEWPORT_YZ].upperLeftCorner.x + 5,
                                state->viewerState->viewPorts[VIEWPORT_YZ].upperLeftCorner.y + 5,
                                200,
                                20);
    AG_WindowShow(win);
    state->viewerState->ag->dataSizeWinyz = win;

    AG_WidgetHide(state->viewerState->ag->dataSizeLabelxy);
    AG_WidgetHide(state->viewerState->ag->dataSizeLabelxz);
    AG_WidgetHide(state->viewerState->ag->dataSizeLabelyz);
}

void createNavWin(struct stateInfo *state) {
	state->viewerState->ag->navWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->navWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->navWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->navWin, "Navigation");
    AG_WindowSetGeometryAligned(state->viewerState->ag->navWin, AG_WINDOW_MR, 250, 200);
    AG_WindowSetCloseAction(state->viewerState->ag->navWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->navWin);
}

void createToolsWin() {
    AG_Textbox *comment;
    AG_Button *button;
    AG_Notebook *toolTabs;
    AG_NotebookTab *quickCommandsTab;
    AG_NotebookTab *treesTab;
    AG_NotebookTab *nodesTab;
    AG_Checkbox *chkBox;

    AG_Box *box;
    AG_Numerical *numerical;

    /* Create window */
	state->viewerState->ag->toolsWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->toolsWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->toolsWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->toolsWin, "tools");
    AG_WindowSetGeometryAligned(state->viewerState->ag->toolsWin, AG_WINDOW_MR, 310, 400);

    /* Create notebook with tabs in window */
    toolTabs = AG_NotebookNew(state->viewerState->ag->toolsWin, AG_NOTEBOOK_EXPAND);

    quickCommandsTab = AG_NotebookAddTab(toolTabs, "Quick", AG_BOX_HOMOGENOUS);
    {
        /* Populate quick commands tab */
        box = AG_BoxNew(quickCommandsTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
        	AG_LabelNewPolled(box, 0, "Tree Count: %i", &state->viewerState->ag->totalTrees);
	        AG_LabelNewPolled(box, 0, "Node Count: %i", &state->viewerState->ag->totalNodes);
        }

        AG_SeparatorNewHoriz(quickCommandsTab);
        numerical = AG_NumericalNewIntR(quickCommandsTab,
                                        0,
                                        NULL,
                                        "Active Tree ID:  ",
                                        &state->viewerState->ag->activeTreeID,
                                        1,
                                        INT_MAX);
        {
            AG_SetEvent(numerical, "numerical-changed", actTreeIDWdgtModified, NULL);
            AG_ExpandHoriz(numerical);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }
        AG_SeparatorNewHoriz(quickCommandsTab);

        numerical = AG_NumericalNewIntR(quickCommandsTab,
                                        0,
                                        NULL,
                                        "Active Node ID: ",
                                        &state->viewerState->ag->activeNodeID,
                                        1,
                                        INT_MAX);
        {
            AG_SetEvent(numerical, "numerical-changed", actNodeIDWdgtModified, NULL);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }

        state->viewerState->ag->actNodeIDWdgt1 = numerical;

        box = AG_BoxNew(quickCommandsTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
        	AG_LabelNewPolled(box, 0, "x: %i", &state->viewerState->ag->activeNodeCoord.x);
    	    AG_LabelNewPolled(box, 0, "y: %i", &state->viewerState->ag->activeNodeCoord.y);
        	AG_LabelNewPolled(box, 0, "z: %i", &state->viewerState->ag->activeNodeCoord.z);
        }

        box = AG_BoxNew(quickCommandsTab, AG_BOX_VERT, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            comment = AG_TextboxNew(box, AG_TEXTBOX_ABANDON_FOCUS, "Comment:  ");
            {
                AG_TextboxSizeHintLines(comment, 1);
                AG_ExpandHoriz(comment);
                AG_TextboxBindASCII(comment, state->viewerState->ag->commentBuffer, 10240);
                AG_SetEvent(comment, "textbox-postchg", actNodeCommentWdgtModified, NULL);
                AG_SetEvent(comment, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(comment, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }

            comment = AG_TextboxNew(box, AG_TEXTBOX_ABANDON_FOCUS, "Search For: ");
            {
                AG_TextboxSizeHintLines(comment, 1);
                AG_ExpandHoriz(comment);
                AG_TextboxBindASCII(comment, state->viewerState->ag->commentSearchBuffer, 2048);
                AG_SetEvent(comment, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(comment, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }

        box = AG_BoxNew(quickCommandsTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);

            button = AG_ButtonNewFn(box, 0, "Find (n)ext", UI_findNextBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }

            button = AG_ButtonNewFn(box, 0, "Find (p)revious", UI_findPrevBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }
        }

        AG_SeparatorNewHoriz(quickCommandsTab);



        AG_LabelNew(quickCommandsTab, 0, "Branch Points");
        AG_ExpandHoriz(AG_LabelNewPolled(quickCommandsTab, 0, "on Stack: %i",
                                         &state->viewerState->ag->numBranchPoints));

        box = AG_BoxNew(quickCommandsTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
           AG_ExpandHoriz(box);
           button = AG_ButtonNewFn(box, 0, "Push (B)ranch Node", UI_pushBranchBtnPressed, NULL, NULL);
           {
               AG_ExpandHoriz(button);
           }
           button = AG_ButtonNewFn(box, 0, "Pop & (J)ump", UI_popBranchBtnPressed, NULL, NULL);
           {
               AG_ExpandHoriz(button);
           }
        }
    }

    treesTab = AG_NotebookAddTab(toolTabs, "Trees", AG_BOX_HOMOGENOUS);
    {
        AG_LabelNewPolled(treesTab, 0, "Tree Count: %i", &state->viewerState->ag->totalTrees);
        numerical = AG_NumericalNewIntR(treesTab, 0, NULL, "Active Tree ID: ", &state->viewerState->ag->activeTreeID, 1, INT_MAX);
        {
            AG_SetEvent(numerical, "numerical-changed", actTreeIDWdgtModified, NULL);
            AG_ExpandHoriz(numerical);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }
        AG_SeparatorNewHoriz(treesTab);
        box = AG_BoxNew(treesTab, AG_BOX_VERT, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_BoxSetPadding(box, 0);
            button = AG_ButtonNewFn(box, 0, "Delete Active Tree", UI_deleteTreeBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }
            button = AG_ButtonNewFn(box, 0, "New Tree (C)", UI_newTreeBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }
        }
        AG_SeparatorNewHoriz(treesTab);
        box = AG_BoxNew(treesTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_BoxSetPadding(box, 0);

            button = AG_ButtonNewFn(box, 0, "Merge Trees", UI_mergeTreesBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }

            numerical = AG_NumericalNewIntR(box, 0, NULL, "ID 1: ", &state->viewerState->ag->mergeTreesID1, 1, INT_MAX);
            {
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }

            numerical = AG_NumericalNewIntR(box, 0, NULL, "ID 2: ", &state->viewerState->ag->mergeTreesID2, 1, INT_MAX);
            {
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }
        AG_SeparatorNewHoriz(treesTab);
        button = AG_ButtonNewFn(treesTab, 0, "Split Tree By Connected Components", UI_splitTreeBtnPressed, NULL, NULL);
        {
            AG_ExpandHoriz(button);
        }
        AG_SeparatorNewHoriz(treesTab);
        box = AG_BoxNew(treesTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_BoxSetPadding(box, 0);
            numerical = AG_NumericalNewFltR(box, 0, NULL, "R: ", &state->viewerState->ag->actTreeColor.r, 0., 1.);
            {
                AG_SetEvent(numerical, "numerical-changed", actTreeColorWdgtModified, NULL);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }

            numerical = AG_NumericalNewFltR(box, 0, NULL, "G: ", &state->viewerState->ag->actTreeColor.g, 0., 1.);
            {
                AG_SetEvent(numerical, "numerical-changed", actTreeColorWdgtModified, NULL);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }

            numerical = AG_NumericalNewFltR(box, 0, NULL, "B: ", &state->viewerState->ag->actTreeColor.b, 0., 1.);
            {
                AG_SetEvent(numerical, "numerical-changed", actTreeColorWdgtModified, NULL);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }

            numerical = AG_NumericalNewFltR(box, 0, NULL, "A: ", &state->viewerState->ag->actTreeColor.a, 0., 1.);
            {
                AG_SetEvent(numerical, "numerical-changed", actTreeColorWdgtModified, NULL);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }
    }

    nodesTab = AG_NotebookAddTab(toolTabs, "Nodes", AG_BOX_HOMOGENOUS);
    {
        AG_LabelNewPolled(nodesTab, 0, "Node count: %i", &state->viewerState->ag->totalNodes);
        numerical = AG_NumericalNewIntR(nodesTab, 0, NULL, "Active Node ID: ", &state->viewerState->ag->activeNodeID, 1, INT_MAX);
        {
            AG_SetEvent(numerical, "numerical-changed", actNodeIDWdgtModified, NULL);
            AG_ExpandHoriz(numerical);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);

            state->viewerState->ag->actNodeIDWdgt2 = numerical;
        }
        box = AG_BoxNew(nodesTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_BoxSetPadding(box, 0);
            button = AG_ButtonNewFn(box, 0, "Jump to node (s)", UI_jumpToNodeBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }
            button = AG_ButtonNewFn(box, 0, "Delete node (Del)", UI_deleteNodeBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }
        }
        box = AG_BoxNew(nodesTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_BoxSetPadding(box, 0);

            button = AG_ButtonNewFn(box, 0, "Link node with (Shift+Click)", UI_linkActiveNodeWithBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }

            numerical = AG_NumericalNewIntR(box, 0, NULL, "ID: ", &state->viewerState->ag->linkActiveWithNode, 1, INT_MAX);
            {
                AG_ExpandHoriz(numerical);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }
        AG_SeparatorNewHoriz(nodesTab);
        box = AG_BoxNew(nodesTab, AG_BOX_VERT, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            comment = AG_TextboxNew(box, AG_TEXTBOX_ABANDON_FOCUS, "Comment:  ");
            {
                AG_TextboxSizeHintLines(comment, 1);
                AG_ExpandHoriz(comment);
                AG_TextboxBindASCII(comment, state->viewerState->ag->commentBuffer, 10240);
                AG_SetEvent(comment, "textbox-postchg", actNodeCommentWdgtModified, NULL);
                AG_SetEvent(comment, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(comment, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }

            comment = AG_TextboxNew(box, AG_TEXTBOX_ABANDON_FOCUS, "Search For: ");
            {
                AG_TextboxSizeHintLines(comment, 1);
                AG_ExpandHoriz(comment);
                AG_TextboxBindASCII(comment, state->viewerState->ag->commentSearchBuffer, 2048);
                AG_SetEvent(comment, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(comment, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }

        box = AG_BoxNew(nodesTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_BoxSetPadding(box, 0);
            button = AG_ButtonNewFn(box, 0, "Find (n)ext", UI_findNextBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }

            button = AG_ButtonNewFn(box, 0, "Find (p)revious", UI_findPrevBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }
        }
        AG_SeparatorNewHoriz(nodesTab);
        AG_CheckboxNewInt(nodesTab, 0, "Use Last Radius as Default", &state->viewerState->ag->useLastActNodeRadiusAsDefault);
        numerical = AG_NumericalNewFltR(nodesTab, 0, NULL, "Active Node Radius (SHIFT+wheel):  ", &state->viewerState->ag->actNodeRadius, 0.1, 500000);
        {
            AG_SetEvent(numerical, "numerical-changed", UI_actNodeRadiusWdgtModified, NULL);
            AG_ExpandHoriz(numerical);
            AG_NumericalSetIncrement (numerical, 0.25);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }
        numerical = AG_NumericalNewFltR(nodesTab, 0, NULL, "Default Node Radius: ", &state->skeletonState->defaultNodeRadius, 0.1, 500000);
        {
            AG_ExpandHoriz(numerical);
            AG_NumericalSetIncrement (numerical, 0.25);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }
        /* move this under viewport settings, it only affects the display
        numerical = AG_NumericalNew(nodesTab, 0, NULL, "Node/Segment Radius Ratio: ");
        {
            AG_ExpandHoriz(numerical);
        }*/
        AG_SeparatorNewHoriz(nodesTab);
        chkBox = AG_CheckboxNewFn(nodesTab, 0, "Enable comment locking", UI_commentLockWdgtModified, NULL, NULL);
        {
            AG_BindInt(chkBox, "state", &state->skeletonState->lockPositions);
        }
        numerical = AG_NumericalNewUint32(nodesTab, 0, NULL, "Locking Radius: ", &state->skeletonState->lockRadius);
        {
            AG_ExpandHoriz(numerical);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }
        AG_LabelNew(nodesTab, 0, "Lock to Nodes With Comment:");
        comment = AG_TextboxNew(nodesTab, AG_TEXTBOX_ABANDON_FOCUS, NULL);
        {
            AG_ExpandHoriz(comment);
            AG_TextboxBindASCII(comment, state->skeletonState->onCommentLock, 1024);
            AG_SetEvent(comment, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(comment, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }
        box = AG_BoxNew(nodesTab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_BoxSetPadding(box, 0);
            button = AG_ButtonNewFn(box, 0, "Lock to Active Node", UI_lockActiveNodeBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }
            button = AG_ButtonNewFn(box, 0, "Disable Locking", UI_disableLockingBtnPressed, NULL, NULL);
            {
                AG_ExpandHoriz(button);
            }
        }
    }

    AG_WindowSetCloseAction(state->viewerState->ag->toolsWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->toolsWin);
}

void createAboutWin() {
    state->viewerState->ag->aboutWin
        = AG_WindowNew(AG_WINDOW_NORESIZE|AG_WINDOW_NOMAXIMIZE|AG_WINDOW_NOMINIMIZE);
    AG_Pixmap *pixmap;
    AG_Surface *splashAG = NULL;
    SDL_Surface *splashSDL = NULL;

    AG_WindowSetCaption(state->viewerState->ag->aboutWin, "About KNOSSOS");
    AG_WindowSetGeometryAligned(state->viewerState->ag->aboutWin, AG_WINDOW_MC, 520, 542);
    AG_WindowSetSideBorders(state->viewerState->ag->aboutWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->aboutWin, 3);

    splashSDL = SDL_LoadBMP("splash");
    splashAG = AG_SurfaceFromSDL(splashSDL);
    if(!splashAG){
        LOG("Error: did not find file splash in KNOSSOS directory. Please reinstall KNOSSOS");
        AG_WindowSetCloseAction(state->viewerState->ag->aboutWin, AG_WINDOW_HIDE);
        AG_WindowHide(state->viewerState->ag->consoleWin);
        return;
    }
    pixmap = AG_PixmapFromSurfaceScaled(state->viewerState->ag->aboutWin, 0, splashAG, 512, 512);

    AG_Expand(pixmap);
    AG_WindowSetCloseAction(state->viewerState->ag->aboutWin, AG_WINDOW_HIDE);
    AG_WindowHide(state->viewerState->ag->aboutWin);
}

void createConsoleWin(struct stateInfo *state) {
    AG_Font *monospace;

    /* The '_' character is not lower than other characters in this font, which works
       around a bug in the AG_Console widget. See agar bug #172.  */
    monospace = AG_FetchFont("_agFontMinimal", 10, -1);

	state->viewerState->ag->consoleWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->consoleWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->consoleWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->consoleWin, "log");
    AG_WindowSetGeometryAligned(state->viewerState->ag->consoleWin, AG_WINDOW_MR, 300, 135);

    state->viewerState->ag->agConsole = AG_ConsoleNew(state->viewerState->ag->consoleWin, AG_CONSOLE_EXPAND|AG_CONSOLE_AUTOSCROLL);
    AG_ConsoleSetFont (state->viewerState->ag->agConsole, monospace);

    AG_Expand(state->viewerState->ag->agConsole);
    AG_ConsoleAppendLine(state->viewerState->ag->agConsole , "Welcome to KNOSSOS 3.1");
    AG_WindowSetCloseAction(state->viewerState->ag->consoleWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->consoleWin);
}

void createDataSetStatsWin(struct stateInfo *state) {
	state->viewerState->ag->dataSetStatsWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->dataSetStatsWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->dataSetStatsWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->dataSetStatsWin, "Dataset Statistics");
    AG_WindowSetGeometryAligned(state->viewerState->ag->dataSetStatsWin, AG_WINDOW_MC, 200, 80);
    AG_WindowSetCloseAction(state->viewerState->ag->dataSetStatsWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->dataSetStatsWin);
}


void createViewPortPrefWin() {
    AG_Box *box1, *box2, *box3;
    AG_Numerical *numerical;
    AG_Button *button;
    AG_Notebook *tabs;
    AG_NotebookTab *tab;
    AG_Radio *radio;
    AG_Checkbox *checkbox;

	state->viewerState->ag->viewPortPrefWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->viewPortPrefWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->viewPortPrefWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->viewPortPrefWin, "Viewport Settings");
    AG_WindowSetGeometryAligned(state->viewerState->ag->viewPortPrefWin, AG_WINDOW_MC, 500, 300);

    /* Create notebook with tabs in window */
    tabs = AG_NotebookNew(state->viewerState->ag->viewPortPrefWin, AG_NOTEBOOK_EXPAND);

    tab = AG_NotebookAddTab(tabs, "General", AG_BOX_HOMOGENOUS);
    {
        box1 = AG_BoxNew(tab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        AG_ExpandHoriz(box1);

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        AG_ExpandHoriz(box2);
        AG_LabelNew(box2, 0, "Skeleton Visualization");

        AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
        AG_BoxSetDepth(box2, 3);
        AG_CheckboxNewInt(box2, 0, "Light effects", &state->viewerState->lightOnOff);
        AG_CheckboxNewFn(box2, 0, "Don't Highlight Active Tree", UI_setHighlightActiveTree, NULL);
        AG_CheckboxNewFn(box2, 0, "Show All Node IDs", UI_setShowNodeIDs, NULL);

        box3 = AG_BoxNew(box2, AG_BOX_HORIZ, 0);
        {
            AG_ExpandHoriz(box3);
            AG_BoxSetPadding(box3, 0);
            AG_BoxSetSpacing(box3, 0);
            checkbox = AG_CheckboxNewInt(box3, 0, "Override Node Radius:", &state->skeletonState->overrideNodeRadiusBool);
            {
                AG_SetEvent(checkbox, "checkbox-changed", UI_skeletonChanged, NULL);
            }
            numerical = AG_NumericalNewFltR(box3,
                                            0,
                                            NULL,
                                            " ",
                                            &state->skeletonState->overrideNodeRadiusVal,
                                            0.,
                                            65536.);
            {
                AG_NumericalSetIncrement(numerical, 0.05);
                AG_SetEvent(numerical, "numerical-changed", UI_skeletonChanged, NULL);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }

        numerical = AG_NumericalNewFltR(box2,
                                        0,
                                        NULL,
                                        "Edge <-> Node Radius Ratio: ",
                                        &state->skeletonState->segRadiusToNodeRadius,
                                        0.,
                                        65536.);
        {
            AG_NumericalSetIncrement(numerical, 0.05);
            AG_SetEvent(numerical, "numerical-changed", UI_skeletonChanged, NULL);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        AG_ExpandHoriz(box2);
        AG_LabelNew(box2, 0, "Skeleton Rendering Model");
        AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
        radio = AG_RadioNewFn(box2, 0, NULL, UI_renderModelRadioModified, NULL, NULL);
        {
            AG_BindInt(radio, "value", &state->viewerState->ag->radioRenderingModel);
            AG_ExpandHoriz(radio);
            AG_RadioAddItem(radio, "Lines and Points (fast)");
            AG_RadioAddItem(radio, "3D Skeleton");
        }

    }
    tab = AG_NotebookAddTab(tabs, "Slice Plane Viewports", AG_BOX_HOMOGENOUS);
    {
        box1 = AG_BoxNew(tab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        AG_ExpandHoriz(box1);

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        AG_ExpandHoriz(box2);
        AG_LabelNew(box2, 0, "Skeleton Overlay");

        AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
        AG_BoxSetDepth(box2, 3);
        checkbox = AG_CheckboxNewInt(box2, 0, "Enable Overlay (1)", &state->viewerState->ag->enableOrthoSkelOverlay);
        {
            AG_SetEvent(checkbox, "checkbox-changed", UI_enableSliceVPOverlayModified, NULL);
        }
        checkbox = AG_CheckboxNewInt(box2, 0, "Highlight Intersections", &state->skeletonState->showIntersections);
        {
            AG_SetEvent(checkbox, "checkbox-changed", UI_skeletonChanged, NULL);
        }
        numerical = AG_NumericalNewFltR(box2, 0, NULL, "Depth Cutoff: ", &state->viewerState->depthCutOff, 0.51, 32.);
        {
            AG_NumericalSetIncrement(numerical, 0.5);
            AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
            AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        }


        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        AG_ExpandHoriz(box2);
        AG_LabelNew(box2, 0, "Voxel Filtering");
        AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
        AG_CheckboxNewFn(box2, AG_CHECKBOX_SET, "Dataset Linear Filtering", UI_enableLinearFilteringModified , NULL);

        box1 = AG_BoxNew(tab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box1);
        }

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        {
            AG_ExpandHoriz(box2);
            AG_LabelNew(box2, 0, "Color lookup tables");
            AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
            checkbox = AG_CheckboxNewInt(box2, 0, "Use own Dataset colors", &state->viewerState->datasetColortableOn);
            {
                AG_SetEvent(checkbox, "checkbox-changed", datasetColorAdjustmentsChanged, NULL);
            }
            checkbox = AG_CheckboxNewInt(box2, 0, "Use own Tree colors", &state->viewerState->treeColortableOn);
            {
                AG_SetEvent(checkbox, "checkbox-changed", treeColorAdjustmentsChanged, NULL);
            }
            button = AG_ButtonNewFn(box2, 0, "Load Dataset Color Lookup Table", createLoadDatasetImgJTableWin, NULL);
            {
                AG_ExpandHoriz(button);
            }
            button = AG_ButtonNewFn(box2, 0, "Load Tree Color Lookup Table", createLoadTreeImgJTableWin, NULL);
            {
                AG_ExpandHoriz(button);
            }
        }

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        {
            AG_ExpandHoriz(box2);
            AG_LabelNew(box2, 0, "Dataset Dynamic Range");
            AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
            numerical = AG_NumericalNewIntR(box2, 0, NULL, "Bias: ", &state->viewerState->luminanceBias, 0, 255);
            {
                AG_SetEvent(numerical, "numerical-changed", datasetColorAdjustmentsChanged, NULL);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
            numerical = AG_NumericalNewIntR(box2, 0, NULL, "Range Delta: ", &state->viewerState->luminanceRangeDelta, 0, 255);
            {
                AG_SetEvent(numerical, "numerical-changed", datasetColorAdjustmentsChanged, NULL);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }

        box1 = AG_BoxNew(tab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box1);
        }

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        {
            AG_LabelNew(box2, 0, "Object ID Overlay");
            AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
            AG_ExpandHoriz(box2);
            AG_CheckboxNewInt(box2, 0, "Enable Color Overlay", &state->viewerState->overlayVisible);
        }

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        {
            AG_LabelNew(box2, 0, "Viewport Objects");
            AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
            AG_ExpandHoriz(box2);
            AG_CheckboxNewInt(box2, 0, "Draw Intersection Crosshairs", &state->viewerState->drawVPCrosshairs);
            AG_CheckboxNewFn(box2, 0, "Show Viewport Size", UI_setShowVPLabels, NULL);
        }
    }
    tab = AG_NotebookAddTab(tabs, "Skeleton Viewport", AG_BOX_HOMOGENOUS);
    {
        box1 = AG_BoxNew(tab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        AG_ExpandHoriz(box1);

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        AG_ExpandHoriz(box2);
        AG_BoxSetDepth(box2, 3);
        AG_LabelNew(box2, 0, "Dataset Visualization");

        AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);

        AG_CheckboxNewInt(box2, 0, "Show XY Plane", &state->skeletonState->showXYplane);
        AG_CheckboxNewInt(box2, 0, "Show XZ Plane", &state->skeletonState->showXZplane);
        AG_CheckboxNewInt(box2, 0, "Show YZ Plane", &state->skeletonState->showYZplane);

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        AG_ExpandHoriz(box2);
        AG_LabelNew(box2, 0, "Skeleton Display Modes");
        AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
        AG_BoxSetDepth(box2, 3);

        radio = AG_RadioNewFn(box2, 0, NULL, UI_displayModeRadioModified, NULL, NULL);
        {
            AG_BindInt(radio, "value", &state->viewerState->ag->radioSkeletonDisplayMode);
            AG_ExpandHoriz(radio);
            AG_RadioAddItem(radio, "Whole Skeleton");
            AG_RadioAddItem(radio, "Only Current Cube");
            AG_RadioAddItem(radio, "Only Active Tree");
            AG_RadioAddItem(radio, "Hide Skeleton (fast)");
        }

        box1 = AG_BoxNew(tab, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        AG_ExpandHoriz(box1);

        box2 = AG_BoxNew(box1, AG_BOX_VERT, 0);
        AG_ExpandHoriz(box2);
        AG_BoxSetDepth(box2, 3);
        AG_LabelNew(box2, 0, "3D View");
        AG_SeparatorSetPadding(AG_SeparatorNewHoriz(box2), 0);
        AG_CheckboxNewInt(box2, 0, "Rotate around Active Node", &state->skeletonState->rotateAroundActiveNode);
    }

    AG_WindowSetCloseAction(state->viewerState->ag->viewPortPrefWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->viewPortPrefWin);
}

void createSetDynRangeWin(struct stateInfo *state) {
    AG_Box *box;
	state->viewerState->ag->setDynRangeWin = AG_WindowNew(0);
    AG_WindowSetSideBorders(state->viewerState->ag->setDynRangeWin, 3);
    AG_WindowSetBottomBorder(state->viewerState->ag->setDynRangeWin, 3);
    AG_WindowSetCaption(state->viewerState->ag->setDynRangeWin, "Set Dynamic Range");
    AG_WindowSetGeometryAligned(state->viewerState->ag->setDynRangeWin, AG_WINDOW_MC, 200, 80);
    box = AG_BoxNew(state->viewerState->ag->setDynRangeWin, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
    AG_ExpandHoriz(box);
    AG_LabelNew(box, 0, "Lower Limit");
    AG_LabelNew(box, 0, "Upper Limit");
    box = AG_BoxNew(state->viewerState->ag->setDynRangeWin, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
    AG_ExpandHoriz(box);
    AG_SliderNew(box, AG_SLIDER_HORIZ, 0);
    AG_SliderNew(box, AG_SLIDER_HORIZ, 0);
    AG_WindowSetCloseAction(state->viewerState->ag->setDynRangeWin, AG_WINDOW_HIDE);
	AG_WindowHide(state->viewerState->ag->setDynRangeWin);
}


void createZoomingWin(struct stateInfo *state) {
    AG_Numerical *numerical;
    AG_Button *button;
    AG_Box *box;
    AG_Window *win;

	win = AG_WindowNew(0);
    {
        AG_WindowSetSideBorders(win, 3);
        AG_WindowSetBottomBorder(win, 3);
        AG_WindowSetCaption(win, "Viewport Zooming");
        AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, 300, 180);

        box = AG_BoxNew(win, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_LabelNew(box, 0, "Viewport XY");

            AG_SliderNewFltR(box,
                             AG_SLIDER_HORIZ,
                             0,
                             &state->viewerState->viewPorts[VIEWPORT_XY].texture.zoomLevel,
                             0.02,
                             1.);
            numerical = AG_NumericalNewFltR(box,
                                            0,
                                            NULL,
                                            "",
                                            &state->viewerState->viewPorts[VIEWPORT_XY].texture.zoomLevel,
                                            0.02,
                                            1.);
            {
                AG_NumericalSetIncrement(numerical, 0.01);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }

        box = AG_BoxNew(win, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_LabelNew(box, 0, "Viewport XZ");

            AG_SliderNewFltR(box,
                             AG_SLIDER_HORIZ,
                             0,
                             &state->viewerState->viewPorts[VIEWPORT_XZ].texture.zoomLevel,
                             0.02,
                             1.);
            numerical = AG_NumericalNewFltR(box,
                                            0,
                                            NULL,
                                            "",
                                            &state->viewerState->viewPorts[VIEWPORT_XZ].texture.zoomLevel,
                                            0.02,
                                            1.);
            {
                AG_NumericalSetIncrement(numerical, 0.01);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }

        box = AG_BoxNew(win, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_LabelNew(box, 0, "Viewport YZ");

            AG_SliderNewFltR(box,
                             AG_SLIDER_HORIZ,
                             0,
                             &state->viewerState->viewPorts[VIEWPORT_YZ].texture.zoomLevel,
                             0.02,
                             1.);
            numerical = AG_NumericalNewFltR(box,
                                            0,
                                            NULL,
                                            "",
                                            &state->viewerState->viewPorts[VIEWPORT_YZ].texture.zoomLevel,
                                            0.02,
                                            1.);
            {
                AG_NumericalSetIncrement(numerical, 0.01);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }

        box = AG_BoxNew(win, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
        {
            AG_ExpandHoriz(box);
            AG_LabelNew(box, 0, "Skeleton View");

            AG_SliderNewFltR(box,
                             AG_SLIDER_HORIZ,
                             0,
                             &state->skeletonState->zoomLevel,
                             0.,
                             0.49);
            numerical = AG_NumericalNewFltR(box,
                                            0,
                                            NULL,
                                            "",
                                            &state->skeletonState->zoomLevel,
                                            0.,
                                            0.49);
            {
                AG_NumericalSetIncrement(numerical, 0.01);
                AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
                AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
            }
        }

        button = AG_ButtonNewFn(win, 0, "All Defaults", UI_setDefaultZoom, NULL);
        {
            AG_ExpandHoriz(button);
        }

        AG_WindowSetCloseAction(win, AG_WINDOW_HIDE);
    	AG_WindowHide(win);
    }

    state->viewerState->ag->zoomingWin = win;
}
void createLoadTreeImgJTableWin() {
    AG_Window *win;
    AG_FileDlg *dlg;

    win = AG_WindowNew(0);
    {
        AG_WindowSetSideBorders(win, 3);
        AG_WindowSetBottomBorder(win, 3);
        AG_WindowSetCaption(win, "Load Tree Color Lookup Table");
        AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, 900, 500);
        AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);
    }

    dlg = AG_FileDlgNew(win, AG_FILEDLG_LOAD);
    {
        AG_Expand(dlg);
        AG_FileDlgAddType(dlg,
                          "",
                          "*.xml",
                          OkfileDlgLoadTreeLUT,
                          NULL);
        AG_FileDlgCancelAction(dlg, CancelFileDlg, NULL);

        AG_FileDlgSetOptionContainer(dlg, AG_BoxNewVert(win, AG_BOX_HFILL));
        AG_FileDlgSetDirectory(dlg, "%s", state->viewerState->ag->treeImgJDirectory);
    }

    AG_WindowShow(win);
}
void createLoadDatasetImgJTableWin() {
    AG_Window *win;
    AG_FileDlg *dlg;

    win = AG_WindowNew(0);
    {
        AG_WindowSetSideBorders(win, 3);
        AG_WindowSetBottomBorder(win, 3);
        AG_WindowSetCaption(win, "Load Dataset Color Lookup Table");
        AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, 900, 500);
        AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);
    }

    dlg = AG_FileDlgNew(win, AG_FILEDLG_LOAD);
    {
        AG_Expand(dlg);
        AG_FileDlgAddType(dlg,
                          "LUT file",
                          "*.*",
                          OkfileDlgLoadDatasetLUT,
                          NULL);
        AG_FileDlgCancelAction(dlg, CancelFileDlg, NULL);

        AG_FileDlgSetOptionContainer(dlg, AG_BoxNewVert(win, AG_BOX_HFILL));
        AG_FileDlgSetDirectory(dlg, "%s", state->viewerState->ag->datasetImgJDirectory);
    }

    AG_WindowShow(win);
}

static void createVpXyWin(struct stateInfo *state) {
	state->viewerState->ag->vpXyWin = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_KEEPBELOW);

    AG_WindowSetCaption(state->viewerState->ag->vpXyWin, "XY");

    AG_WindowSetGeometry(state->viewerState->ag->vpXyWin,
        5,
        30,
        state->viewerState->viewPorts[0].edgeLength,
        state->viewerState->viewPorts[0].edgeLength);

    AG_WindowSetPadding(state->viewerState->ag->vpXyWin, 0, 0, 0, 0);
    state->viewerState->ag->glViewXy = AG_GLViewNew(state->viewerState->ag->vpXyWin, 0);
    //AGWIDGET(state->viewerState->ag->glViewXy)->flags |= AG_WIDGET_UNFOCUSED_MOTION;
    AG_Expand(state->viewerState->ag->glViewXy);
    AG_GLViewDrawFn(state->viewerState->ag->glViewXy, drawXyViewport, "%p", state);
    //AG_GLViewMotionFn(state->viewerState->ag->glViewXy, mouseVpXYmotion, NULL);

    AG_WindowSetCloseAction(state->viewerState->ag->vpXyWin, AG_WINDOW_HIDE);
	AG_WindowShow(state->viewerState->ag->vpXyWin);
}

static void createVpXzWin(struct stateInfo *state) {
	state->viewerState->ag->vpXzWin = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_KEEPBELOW);
    AG_WindowSetCaption(state->viewerState->ag->vpXzWin, "XZ");
    AG_WindowSetGeometry(state->viewerState->ag->vpXzWin,
        5,
        35 + state->viewerState->viewPorts[0].edgeLength,
        state->viewerState->viewPorts[1].edgeLength,
        state->viewerState->viewPorts[1].edgeLength);

    AG_WindowSetPadding(state->viewerState->ag->vpXzWin, 0, 0, 0, 0);
    state->viewerState->ag->glViewXz = AG_GLViewNew(state->viewerState->ag->vpXzWin, 0);
    //AGWIDGET(state->viewerState->ag->glViewXz)->flags |= AG_WIDGET_UNFOCUSED_MOTION;
    AG_Expand(state->viewerState->ag->glViewXz);

    AG_GLViewDrawFn(state->viewerState->ag->glViewXz, drawXzViewport, "%p", state);
    //AG_GLViewMotionFn(state->viewerState->ag->glViewXz, mouseVpXZmotion, NULL);

    AG_WindowSetCloseAction(state->viewerState->ag->vpXzWin, AG_WINDOW_HIDE);
	AG_WindowShow(state->viewerState->ag->vpXzWin);
}

static void createVpYzWin(struct stateInfo *state) {
	state->viewerState->ag->vpYzWin = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_KEEPBELOW);
    AG_WindowSetCaption(state->viewerState->ag->vpYzWin, "yz");
    AG_WindowSetGeometry(state->viewerState->ag->vpYzWin,
        10 + state->viewerState->viewPorts[0].edgeLength,
        30,
        state->viewerState->viewPorts[2].edgeLength,
        state->viewerState->viewPorts[2].edgeLength);

    AG_WindowSetPadding(state->viewerState->ag->vpYzWin, 0, 0, 0, 0);
    state->viewerState->ag->glViewYz = AG_GLViewNew(state->viewerState->ag->vpYzWin, 0);
    //AGWIDGET(state->viewerState->ag->glViewYz)->flags |= AG_WIDGET_UNFOCUSED_MOTION;
    AG_Expand(state->viewerState->ag->glViewYz);
    AG_GLViewDrawFn(state->viewerState->ag->glViewYz, drawYzViewport, "%p", state);
    //AG_GLViewMotionFn(state->viewerState->ag->glViewYz, mouseVpYZmotion, NULL);

    AG_WindowSetCloseAction(state->viewerState->ag->vpYzWin, AG_WINDOW_HIDE);
	AG_WindowShow(state->viewerState->ag->vpYzWin);
}

static void createVpSkelWin(struct stateInfo *state) {
	state->viewerState->ag->vpSkelWin = AG_WindowNew(AG_WINDOW_PLAIN|AG_WINDOW_KEEPBELOW);
    AG_WindowSetCaption(state->viewerState->ag->vpSkelWin, "Skeleton View");
    AG_WindowSetGeometry(state->viewerState->ag->vpSkelWin,
        10 + state->viewerState->viewPorts[1].edgeLength,
        35 + state->viewerState->viewPorts[2].edgeLength,
        state->viewerState->viewPorts[3].edgeLength,
        state->viewerState->viewPorts[3].edgeLength);

    AG_WindowSetPadding(state->viewerState->ag->vpSkelWin, 0, 0, 0, 0);
    state->viewerState->ag->glViewSkel = AG_GLViewNew(state->viewerState->ag->vpSkelWin, 0);
    AG_Expand(state->viewerState->ag->glViewSkel);
    AG_GLViewDrawFn(state->viewerState->ag->glViewSkel, drawSkelViewport, "%p", state);
    AG_WindowSetCloseAction(state->viewerState->ag->vpSkelWin, AG_WINDOW_HIDE);
	AG_WindowShow(state->viewerState->ag->vpSkelWin);
}

static void createOpenFileDlgWin(struct stateInfo *state) {
    AG_Window *win;
    AG_FileDlg *dlg;
    AG_FileType *type;
    agInputWdgtGainedFocus(NULL);

	win = AG_WindowNew(0);
    {
        AG_WindowSetSideBorders(win, 3);
        AG_WindowSetBottomBorder(win, 3);
        AG_WindowSetCaption(win, "Open Skeleton File");
        AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, 900, 500);
        AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);
    }

    dlg = AG_FileDlgNew(win, AG_FILEDLG_LOAD);
    {
        AG_Expand(dlg);
        type = AG_FileDlgAddType(dlg,
                                 "KNOSSOS Skeleton File",
                                 "*.nml",
                                 OkfileDlgOpenSkel,
                                 NULL);
        {
            AG_FileOptionNewBool(type, "Merge With Open Skeleton", "merge", 0);
        }

        AG_FileDlgSetDirectory(dlg, "%s", state->viewerState->ag->skeletonDirectory);
        AG_FileDlgCancelAction(dlg, CancelFileDlg, NULL);

        AG_FileDlgSetOptionContainer(dlg, AG_BoxNewVert(win, AG_BOX_HFILL));
    }

    state->viewerState->ag->fileTypeNml = type;

    AG_AddEvent(win, "window-hidden", agInputWdgtLostFocus, NULL);
    AG_AddEvent(win, "window-shown", agInputWdgtGainedFocus, NULL);
    AG_WindowShow(win);
}

static void createSaveAsFileDlgWin(struct stateInfo *state) {
    AG_Window *win;
    AG_FileDlg *dlg;

    win = AG_WindowNew(0);
    {
        AG_WindowSetSideBorders(win, 3);
        AG_WindowSetBottomBorder(win, 3);
        AG_WindowSetCaption(win, "Save Skeleton File");
        AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, 900, 500);
        AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);
    }

    dlg = AG_FileDlgNew(win, AG_FILEDLG_CLOSEWIN);
    {
        AG_Expand(dlg);
        AG_FileDlgAddType(dlg,
                          "KNOSSOS Skeleton File",
                          "*.nml",
                          OkfileDlgSaveAsSkel,
                          NULL);

        AG_FileDlgSetDirectory(dlg, "%s", state->viewerState->ag->skeletonDirectory);
        AG_FileDlgCancelAction(dlg, CancelFileDlg, NULL);

        AG_FileDlgSetOptionContainer(dlg, AG_BoxNewVert(win, AG_BOX_HFILL));
    }

    AG_AddEvent(win, "window-hidden", agInputWdgtLostFocus, NULL);
    AG_AddEvent(win, "window-shown", agInputWdgtGainedFocus, NULL);

    AG_WindowShow(win);
}

static void createOpenCustomPrefsDlgWin() {
    AG_Window *win;
    AG_FileDlg *dlg;

	win = AG_WindowNew(0);
    {
        AG_WindowSetSideBorders(win, 3);
        AG_WindowSetBottomBorder(win, 3);
        AG_WindowSetCaption(win, "Open Custom Preferences File");
        AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, 900, 500);
        AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);
    }

    dlg = AG_FileDlgNew(win, AG_FILEDLG_LOAD);
    {
        AG_Expand(dlg);
        AG_FileDlgAddType(dlg,
                          "KNOSSOS GUI preferences File",
                          "*.xml",
                          OkfileDlgOpenPrefs,
                          NULL);

        AG_FileDlgSetDirectory(dlg, "%s", state->viewerState->ag->customPrefsDirectory);
        AG_FileDlgCancelAction(dlg, CancelFileDlg, NULL);

        AG_FileDlgSetOptionContainer(dlg, AG_BoxNewVert(win, AG_BOX_HFILL));
    }

    AG_AddEvent(win, "window-hidden", agInputWdgtLostFocus, NULL);
    AG_AddEvent(win, "window-shown", agInputWdgtGainedFocus, NULL);
    AG_WindowShow(win);

}

static void createSaveCustomPrefsAsDlgWin() {
    AG_Window *win;
    AG_FileDlg *dlg;

	win = AG_WindowNew(0);
    {
        AG_WindowSetSideBorders(win, 3);
        AG_WindowSetBottomBorder(win, 3);
        AG_WindowSetCaption(win, "Save Custom Preferences File As");
        AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, 900, 500);
        AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);
    }

    dlg = AG_FileDlgNew(win, AG_FILEDLG_SAVE);
    {
        AG_Expand(dlg);
        AG_FileDlgAddType(dlg,
                          "KNOSSOS GUI preferences File",
                          "*.xml",
                          OkfileDlgSavePrefsAs,
                          NULL);

        AG_FileDlgSetDirectory(dlg, "%s", state->viewerState->ag->customPrefsDirectory);
        AG_FileDlgCancelAction(dlg, CancelFileDlg, NULL);

        AG_FileDlgSetOptionContainer(dlg, AG_BoxNewVert(win, AG_BOX_HFILL));
    }

    AG_AddEvent(win, "window-hidden", agInputWdgtLostFocus, NULL);
    AG_AddEvent(win, "window-shown", agInputWdgtGainedFocus, NULL);

    AG_WindowShow(win);
}

void createSkeletonVpToolsWdgt(AG_Window *parent) {
    AG_Box *box;
    AG_Button *button;

    box = AG_BoxNew(parent, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
    AG_ExpandHoriz(box);
    AG_BoxSetPadding(box, 0);
    AG_BoxSetSpacing(box, 0);

    button = AG_ButtonNewFn(box, 0, "xy", UI_setSkeletonPerspective, "%i", 1);
    {
        AG_ButtonSetPadding(button, 1, 1, 1, 1);
        AG_ExpandHoriz(button);
    }

    button = AG_ButtonNewFn(box, 0, "xz", UI_setSkeletonPerspective, "%i", 2);
    {
        AG_ButtonSetPadding(button, 1, 1, 1, 1);
        AG_ExpandHoriz(button);
    }

    button = AG_ButtonNewFn(box, 0, "yz", UI_setSkeletonPerspective, "%i", 3);
    {
        AG_ButtonSetPadding(button, 1, 1, 1, 1);
        AG_ExpandHoriz(button);
    }

    button = AG_ButtonNewFn(box, 0, "flip", UI_setSkeletonPerspective, "%i", 4);
    {
        AG_ButtonSetPadding(button, 1, 1, 1, 1);
        AG_ExpandHoriz(button);
    }

        button = AG_ButtonNewFn(box, 0, "reset", UI_setSkeletonPerspective, "%i", 5);
    {
        AG_ButtonSetPadding(button, 1, 1, 1, 1);
        AG_ExpandHoriz(button);
    }
}

void createCurrPosWdgt(AG_Window *parent) {
    AG_Box *box;
    AG_Numerical *numerical;
    AG_Button *button;

    box = AG_BoxNew(parent, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS);
    AG_ExpandHoriz(box);
    AG_BoxSetPadding(box, 0);
    AG_BoxSetSpacing(box, 0);

    button = AG_ButtonNewFn(box, 0, "copy", UI_copyClipboardCoordinates, NULL);
    {
        AG_ButtonSetPadding(button, 1, 1, 1, 1);
        AG_ExpandHoriz(button);
    }

    button = AG_ButtonNewFn(box, 0, "paste", UI_pasteClipboardCoordinates, NULL);
    {
        AG_ButtonSetPadding(button, 1, 1, 1, 1);
        AG_ExpandHoriz(button);
    }

    numerical = AG_NumericalNewIntR(box, 0, NULL, " x ", &state->viewerState->ag->oneShiftedCurrPos.x, 1, INT_MAX);
    {
        AG_SetEvent(numerical, "numerical-changed", currPosWdgtModified, NULL);
        AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
        AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
        AG_WidgetSetSize(numerical, 100, 15);
    }


    numerical = AG_NumericalNewIntR(box, 0, NULL, " y ", &state->viewerState->ag->oneShiftedCurrPos.y, 1, INT_MAX);
    {
        AG_SetEvent(numerical, "numerical-changed", currPosWdgtModified, NULL);
        AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
        AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
    }

    numerical = AG_NumericalNewIntR(box, 0, NULL, " z ", &state->viewerState->ag->oneShiftedCurrPos.z, 1, INT_MAX);
    {
        AG_SetEvent(numerical, "numerical-changed", currPosWdgtModified, NULL);
        AG_SetEvent(numerical, "widget-gainfocus", agInputWdgtGainedFocus, NULL);
        AG_SetEvent(numerical, "widget-lostfocus", agInputWdgtLostFocus, NULL);
    }

}



void createActNodeWdgt(struct stateInfo *state, AG_Widget *parent) {

    //AG_Editable *tb;

    //*buffers = "123";

    //tb = AG_EditableNew(parent, AG_EDITABLE_INT_ONLY);
    //AG_EditableBindUTF8(tb, buffers, sizeof(buffers));


}

static void UI_checkQuitKnossos() {
    SDL_Event quitEvent;

    quitEvent.type = SDL_QUIT;
    SDL_PushEvent(&quitEvent);
}

void quitKnossos() {
    /* If you want to react to a user action by quitting,
       go through UI_checkQuitKnossos. */

    SDL_Event quitUserEvent;

    quitUserEvent.type = SDL_USEREVENT;
    quitUserEvent.user.code = USEREVENT_REALQUIT;
    SDL_PushEvent(&quitUserEvent);
}

/*static void viewDataSetStats(AG_Event *event) {
    TODO

    AG_WindowShow(state->viewerState->ag->dataSetStatsWin);
}*/


static void viewZooming(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->zoomingWin);
}

/*static void viewLoadImgJTable(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->loadImgJTableWin);
}


TDitem TODO
static void prefAGoptions(AG_Event *event) {
}


static void prefDispOptions(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->dispOptWin);
}*/

static void prefNavOptions(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->navOptWin);
}

static void prefSyncOptions(AG_Event *event) {
    if(state->clientState->connected)
        AG_LabelText(state->viewerState->ag->syncOptLabel, "Connected to server.");
    else
        AG_LabelText(state->viewerState->ag->syncOptLabel, "No connection to server.");

    AG_WindowShow(state->viewerState->ag->syncOptWin);
}

/*static void prefRenderingQualityOptions(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->renderingOptWin);
}

static void prefSpatialLockingOptions(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->spatLockOptWin);
}

static void prefVolTracingOptions(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->volTraceOptWin);
}*/

static void prefSaveOptions(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->saveOptWin);
}

static void prefViewportPrefs() {
    AG_WindowShow(state->viewerState->ag->viewPortPrefWin);
}

static void prefLoadCustomPrefs(AG_Event *event) {
    createOpenCustomPrefsDlgWin();
}

static void prefSaveCustomPrefsAs(AG_Event *event) {
    createSaveCustomPrefsAsDlgWin();
}

/*static void winShowNavigator(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->navWin);
}*/

static void winShowTools(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->toolsWin);
}

static void winShowConsole(AG_Event *event) {
    AG_WindowShow(state->viewerState->ag->consoleWin);
}

static void fileOpenSkelFile(AG_Event *event) {
    createOpenFileDlgWin(state);
}

static void fileSaveAsSkelFile(AG_Event *event) {
    createSaveAsFileDlgWin(state);
}

/* GUI event handler functions */

static void CancelFileDlg(AG_Event *event) {
    AG_FileDlg *dlg = AG_SELF();

    AG_ObjectDetach(AG_WidgetParentWindow(dlg));
}

void yesNoPrompt(AG_Widget *par, char *promptString, void (*yesCb)(), void (*noCb)()) {
    AG_Button *btns[2];
    AG_Window *win;

    win = AG_TextPromptOptions(btns, 2, "%s", promptString);

    AG_ButtonText(btns[0], "Yes");
    AG_SetEvent(btns[0], "button-pushed", yesNoPromptHitYes, "%p,%p,%p", win, yesCb, par);

    AG_ButtonText(btns[1], "No");
    AG_SetEvent(btns[1], "button-pushed", yesNoPromptHitNo, "%p,%p,%p", win, noCb, par);
}

void UI_zoomOrthogonals(float step) {
    int32_t i = 0;

    for(i = 0; i < state->viewerState->numberViewPorts; i++) {
        if(state->viewerState->viewPorts[i].type != VIEWPORT_SKELETON) {
            if(!(state->viewerState->viewPorts[i].texture.zoomLevel + step < 0.09999) &&
               !(state->viewerState->viewPorts[i].texture.zoomLevel + step > 1.00001)) {
                state->viewerState->viewPorts[i].texture.zoomLevel += step;
            }
        }
    }

    /* TDitem */
    recalcTextureOffsets();
    drawGUI(state);
}
void UI_saveSkeleton(int32_t increment) {
    FILE *saveFile;

    if(increment) {
        increment = state->skeletonState->autoFilenameIncrementBool;
    }

    updateSkeletonFileName(CHANGE_MANUAL,
                           increment,
                           state->skeletonState->skeletonFile);

    saveFile = fopen(state->skeletonState->skeletonFile, "r");
    if(saveFile) {
        yesNoPrompt(NULL, "Overwrite existing skeleton file?", WRAP_saveSkeleton, NULL);
        fclose(saveFile);
        return;
    }

    WRAP_saveSkeleton();
}

void WRAP_saveSkeleton() {
    if(saveSkeleton() == -1) {
        AG_TextError("The skeleton was not saved successfully.\n"
                     "Check disk space and access rights.\n"
                     "Attempted to write to: %s", state->skeletonState->skeletonFile);
        LOG("Save to %s failed.", state->skeletonState->skeletonFile);
    }
    else {
        updateTitlebar(TRUE);
        LOG("Successfully saved to %s", state->skeletonState->skeletonFile);
        state->skeletonState->unsavedChanges = FALSE;
    }
}

void OkfileDlgLoadTreeLUT(AG_Event *event) {
    AG_FileDlg *dlg = AG_SELF();
    char *filename = AG_STRING(1);

    cpBaseDirectory(state->viewerState->ag->treeLUTDirectory, filename, 2048);
    state->viewerState->treeLutSet = TRUE;
    if(loadTreeColorTable(filename, &(state->viewerState->treeColortable[0]), state) != TRUE) {
        LOG("Error loading Tree LUT.\n");
        memcpy(&(state->viewerState->treeColortable[0]),
               &(state->viewerState->neutralTreeTable[0]),
                 state->viewerState->treeLutSize);
        state->viewerState->treeLutSet = FALSE;
    }

    treeColorAdjustmentsChanged();

    AG_ObjectDetach(AG_WidgetParentWindow(dlg));
}
void OkfileDlgLoadDatasetLUT(AG_Event *event) {
    AG_FileDlg *dlg = AG_SELF();
    char *filename = AG_STRING(1);

    cpBaseDirectory(state->viewerState->ag->datasetLUTDirectory, filename, 2048);

    if(loadDatasetColorTable(filename, &(state->viewerState->datasetColortable[0][0]), GL_RGB, state) != TRUE) {
        LOG("Error loading Dataset LUT.\n");
        memcpy(&(state->viewerState->datasetColortable[0][0]),
               &(state->viewerState->neutralDatasetTable[0][0]),
               768);
    }

    datasetColorAdjustmentsChanged();

    AG_ObjectDetach(AG_WidgetParentWindow(dlg));
}

void OkfileDlgOpenSkel(AG_Event *event) {
    AG_FileDlg *dlg = AG_SELF();
    char *filename = AG_STRING(1);
    AG_Event ev;

    cpBaseDirectory(state->viewerState->ag->skeletonDirectory, filename, 2048);

	state->skeletonState->mergeOnLoadFlag = AG_FileOptionBool(state->viewerState->ag->fileTypeNml, "merge");

    if(state->skeletonState->mergeOnLoadFlag) {
        AG_EventArgs(&ev,
                     "%s,%s",
                     filename,
                     "Do you really want to merge the new skeleton into the currently loaded one?");
    }

    else {
        AG_EventArgs(&ev,
                     "%s,%s",
                     filename,
                     "This will overwrite the currently loaded skeleton. Are you sure?");
    }

    UI_loadSkeleton(&ev);

    AG_ObjectDetach(AG_WidgetParentWindow(dlg));
}

void OkfileDlgSaveAsSkel(AG_Event *event) {
    char *filename = AG_STRING(1);

    /*
     *  TODO Sanity check path.
     */

    cpBaseDirectory(state->viewerState->ag->skeletonDirectory, filename, 2048);

    strncpy(state->skeletonState->skeletonFile, filename, 8192);

    UI_saveSkeleton(FALSE);
}

void OkfileDlgOpenPrefs(AG_Event *event) {
    AG_FileDlg *dlg = AG_SELF();
	char *filename = AG_STRING(1);

    cpBaseDirectory(state->viewerState->ag->customPrefsDirectory, filename, 2048);

    strncpy(state->viewerState->ag->settingsFile, filename, 2048);
    UI_loadSettings();

    AG_ObjectDetach(AG_WidgetParentWindow(dlg));
}

void OkfileDlgSavePrefsAs(AG_Event *event) {
    AG_FileDlg *dlg = AG_SELF();
	char *filename = AG_STRING(1);

    cpBaseDirectory(state->viewerState->ag->customPrefsDirectory, filename, 2048);

    strncpy(state->viewerState->ag->settingsFile, filename, 2048);
    UI_saveSettings();

    AG_ObjectDetach(AG_WidgetParentWindow(dlg));
}

static void UI_commentLockWdgtModified(AG_Event *event) {
    int lockOn = AG_INT(1);
    if(lockOn) {
        state->skeletonState->lockPositions = TRUE;
    }
    else {
        state->skeletonState->lockPositions = FALSE;
        state->skeletonState->positionLocked = FALSE;
    }
}


void actNodeWdgtChanged(AG_Event *event)
{
	char *s = AG_STRING(1);    /* Given in AG_SetEvent() */
	int new_state = AG_INT(2); /* Passed by 'button-pushed',
	                              see AG_Button(3) */
	AG_TextMsg(AG_MSG_INFO, "Hello, %s! (state = %d)",
	    s, new_state);
}

static void drawXyViewport(AG_Event *event) {
    renderOrthogonalVP(0, state);
}

static void drawXzViewport(AG_Event *event) {
    renderOrthogonalVP(1, state);
}

static void drawYzViewport(AG_Event *event) {
    renderOrthogonalVP(2, state);
}

static void drawSkelViewport(AG_Event *event) {
    renderSkeletonVP(3, state);
}

static void resizeCallback(uint32_t newWinLenX, uint32_t newWinLenY) {
    uint32_t i = 0;

    /* calculate new size of VP windows.
    Enlarging win again does not work properly TDitem */

    /* find out whether we're x or y limited with our window */
    if(newWinLenX <= newWinLenY) {
        for(i = 0; i < state->viewerState->numberViewPorts; i++)
            state->viewerState->viewPorts[i].edgeLength = newWinLenX / 2 - 50;
    }
    else {
            for(i = 0; i < state->viewerState->numberViewPorts; i++)
        state->viewerState->viewPorts[i].edgeLength = newWinLenY / 2 - 50;
    }


    SET_COORDINATE(state->viewerState->viewPorts[VIEWPORT_XY].upperLeftCorner,
        5, 30, 0);

    SET_COORDINATE(state->viewerState->viewPorts[VIEWPORT_XZ].upperLeftCorner,
        5,
        state->viewerState->viewPorts[VIEWPORT_XY].upperLeftCorner.y
        + state->viewerState->viewPorts[VIEWPORT_XY].edgeLength + 5,
        0);

    SET_COORDINATE(state->viewerState->viewPorts[VIEWPORT_YZ].upperLeftCorner,
        state->viewerState->viewPorts[VIEWPORT_XY].edgeLength + 10,
        30,
        0);

    SET_COORDINATE(state->viewerState->viewPorts[VIEWPORT_SKELETON].upperLeftCorner,
        state->viewerState->viewPorts[VIEWPORT_XZ].edgeLength + 10,
        35 + state->viewerState->viewPorts[VIEWPORT_YZ].edgeLength,
        0);

    state->viewerState->screenSizeX = newWinLenX;
    state->viewerState->screenSizeY = newWinLenY;

    /* reposition & resize VP windows */
    AG_WindowSetGeometry(state->viewerState->ag->vpXyWin,
        state->viewerState->viewPorts[VIEWPORT_XY].upperLeftCorner.x,
        state->viewerState->viewPorts[VIEWPORT_XY].upperLeftCorner.y,
        state->viewerState->viewPorts[VIEWPORT_XY].edgeLength,
        state->viewerState->viewPorts[VIEWPORT_XY].edgeLength);

    AG_WindowSetGeometry(state->viewerState->ag->vpXzWin,
        state->viewerState->viewPorts[VIEWPORT_XZ].upperLeftCorner.x,
        state->viewerState->viewPorts[VIEWPORT_XZ].upperLeftCorner.y,
        state->viewerState->viewPorts[VIEWPORT_XZ].edgeLength,
        state->viewerState->viewPorts[VIEWPORT_XZ].edgeLength);

    AG_WindowSetGeometry(state->viewerState->ag->vpYzWin,
        state->viewerState->viewPorts[VIEWPORT_YZ].upperLeftCorner.x,
        state->viewerState->viewPorts[VIEWPORT_YZ].upperLeftCorner.y,
        state->viewerState->viewPorts[VIEWPORT_YZ].edgeLength,
        state->viewerState->viewPorts[VIEWPORT_YZ].edgeLength);

    AG_WindowSetGeometry(state->viewerState->ag->vpSkelWin,
        state->viewerState->viewPorts[VIEWPORT_SKELETON].upperLeftCorner.x,
        state->viewerState->viewPorts[VIEWPORT_SKELETON].upperLeftCorner.y,
        state->viewerState->viewPorts[VIEWPORT_SKELETON].edgeLength,
        state->viewerState->viewPorts[VIEWPORT_SKELETON].edgeLength);

    /* reposition & resize current pos window (in title bar!) */
    AG_WindowSetGeometryAligned(state->viewerState->ag->coordBarWin,
        AG_WINDOW_TR, 500, 20);

    AG_WindowSetGeometryBounded(state->viewerState->ag->skeletonVpToolsWin,
                                state->viewerState->viewPorts[VIEWPORT_SKELETON].upperLeftCorner.x + state->viewerState->viewPorts[VIEWPORT_SKELETON].edgeLength - 160,
                                state->viewerState->viewPorts[VIEWPORT_SKELETON].upperLeftCorner.y + 5,
                                150,
                                20);
    /* reinit the rendering system - resizing the window leads to a new OGL context */
    initializeTextures();
    /* deleting the display lists here works, but I don't understand why its nec.
    it should be sufficient to set the change flags to TRUE */
    if(state->skeletonState->displayListSkeletonSkeletonizerVP) {
        glDeleteLists(state->skeletonState->displayListSkeletonSkeletonizerVP, 1);
        state->skeletonState->displayListSkeletonSkeletonizerVP = 0;
    }
    if(state->skeletonState->displayListSkeletonSlicePlaneVP) {
        glDeleteLists(state->skeletonState->displayListSkeletonSlicePlaneVP, 1);
        state->skeletonState->displayListSkeletonSlicePlaneVP = 0;
    }
    if(state->skeletonState->displayListView) {
        glDeleteLists(state->skeletonState->displayListView, 1);
        state->skeletonState->displayListView = 0;
    }
    if(state->skeletonState->displayListDataset) {
        glDeleteLists(state->skeletonState->displayListDataset, 1);
        state->skeletonState->displayListDataset = 0;
    }
    /* all display lists have to be rebuilt */
    state->skeletonState->viewChanged = TRUE;
    state->skeletonState->skeletonChanged = TRUE;
    state->skeletonState->datasetChanged = TRUE;
    state->skeletonState->skeletonSliceVPchanged = TRUE;
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    //updateDisplayListsSkeleton(state);

    AG_WindowSetGeometryBounded(state->viewerState->ag->dataSizeWinxy,
                                state->viewerState->viewPorts[VIEWPORT_XY].upperLeftCorner.x + 5,
                                state->viewerState->viewPorts[VIEWPORT_XY].upperLeftCorner.y + 5,
                                200,
                                20);
    AG_WindowSetGeometryBounded(state->viewerState->ag->dataSizeWinxz,
                                state->viewerState->viewPorts[VIEWPORT_XZ].upperLeftCorner.x + 5,
                                state->viewerState->viewPorts[VIEWPORT_XZ].upperLeftCorner.y + 5,
                                200,
                                20);
    AG_WindowSetGeometryBounded(state->viewerState->ag->dataSizeWinyz,
                                state->viewerState->viewPorts[VIEWPORT_YZ].upperLeftCorner.x + 5,
                                state->viewerState->viewPorts[VIEWPORT_YZ].upperLeftCorner.y + 5,
                                200,
                                20);

    drawGUI();

}

static void currPosWdgtModified(AG_Event *event) {
    tempConfig->viewerState->currentPosition.x =
        state->viewerState->ag->oneShiftedCurrPos.x - 1;
    tempConfig->viewerState->currentPosition.y =
        state->viewerState->ag->oneShiftedCurrPos.y - 1;
    tempConfig->viewerState->currentPosition.z =
        state->viewerState->ag->oneShiftedCurrPos.z - 1;

    updatePosition(state, TELL_COORDINATE_CHANGE);
}

static void actNodeIDWdgtModified(AG_Event *event) {
    setActiveNode(CHANGE_MANUAL,
                  NULL,
                  state->viewerState->ag->activeNodeID);
}


static void actTreeIDWdgtModified(AG_Event *event) {
    setActiveTreeByID(state->viewerState->ag->activeTreeID, state);
}

static void actTreeColorWdgtModified(AG_Event *event) {
    if(state->skeletonState->activeTree) {
        state->skeletonState->activeTree->color =
            state->viewerState->ag->actTreeColor;
    }
}


static void actNodeCommentWdgtModified(AG_Event *event) {

    strncpy(state->skeletonState->commentBuffer,
        state->viewerState->ag->commentBuffer,
        10240);

    if(strlen(state->viewerState->ag->commentBuffer) == 0) {
        delComment(CHANGE_MANUAL, state->skeletonState->currentComment, 0, state);
        return;
    }
    if(state->skeletonState->activeNode) {
        if(!state->skeletonState->activeNode->comment)
            addComment(CHANGE_MANUAL,
                        state->skeletonState->commentBuffer,
                        state->skeletonState->activeNode,
                        0,
                        state);
        else
            editComment(CHANGE_MANUAL,
                        state->skeletonState->activeNode->comment,
                        0,
                        state->skeletonState->commentBuffer,
                        state->skeletonState->activeNode,
                        0,
                        state);
    }
    /*state->skeletonState->skeletonChanged = TRUE; needed? TDitem */
}

static void UI_findNextBtnPressed() {
    nextComment(state->viewerState->ag->commentSearchBuffer, state);
}

static void UI_findPrevBtnPressed() {
    previousComment(state->viewerState->ag->commentSearchBuffer, state);
}

static void agInputWdgtGainedFocus(AG_Event *event) {
    state->viewerState->ag->agInputWdgtFocused = TRUE;
}

static void agInputWdgtLostFocus(AG_Event *event) {
    state->viewerState->ag->agInputWdgtFocused = FALSE;
}


/*
 *  Wrapper functions around KNOSSOS internals for use by the UI
 * (GUI / Keyboard / (Mouse))
 *
 */

void UI_workModeAdd() {
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
}

void UI_workModeLink() {
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;
}

void UI_workModeDrop() {
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_DROP_NODE;
}

static void UI_clearSkeleton() {
    yesNoPrompt(NULL,
                "Really clear the skeleton (you can not undo this)?",
                WRAP_clearSkeleton,
                NULL);
}

static void WRAP_clearSkeleton() {
    clearSkeleton(CHANGE_MANUAL);
}

static void UI_setViewModeDrag() {
    tempConfig->viewerState->workMode = ON_CLICK_DRAG;
}

static void UI_setViewModeRecenter() {
    tempConfig->viewerState->workMode = ON_CLICK_RECENTER;
}

static void UI_unimplemented() {
    AG_TextMsg(AG_MSG_INFO, "This feature is not yet implemented.");
}

static void UI_setDefaultZoom() {
    state->viewerState->viewPorts[VIEWPORT_XY].texture.zoomLevel = 1.;
    state->viewerState->viewPorts[VIEWPORT_XZ].texture.zoomLevel = 1.;
    state->viewerState->viewPorts[VIEWPORT_YZ].texture.zoomLevel = 1.;
    state->skeletonState->zoomLevel = 0.;
}

static void UI_SyncConnect() {
    if(!state->clientState->connected)
        sendClientSignal(state);
}

static void UI_SyncDisconnect() {
    if(state->clientState->connected)
        sendClientSignal(state);
}

static void UI_lockActiveNodeBtnPressed() {
    Coordinate activeNodePosition;

    if(state->skeletonState->activeNode) {
        LOG("Locking to active node.");

        activeNodePosition.x = state->skeletonState->activeNode->position.x;
        activeNodePosition.y = state->skeletonState->activeNode->position.y;
        activeNodePosition.z = state->skeletonState->activeNode->position.z;

        lockPosition(activeNodePosition, state);
    }
    else
        LOG("There is no active node to lock.");
}

static void UI_disableLockingBtnPressed() {
    unlockPosition(state);
}

static void UI_deleteNodeBtnPressed() {
    delActiveNode(state);
}

static void UI_jumpToNodeBtnPressed() {
    if(state->skeletonState->activeNode) {
        tempConfig->viewerState->currentPosition.x =
            state->skeletonState->activeNode->position.x /
            state->magnification;
        tempConfig->viewerState->currentPosition.y =
            state->skeletonState->activeNode->position.y /
            state->magnification;
        tempConfig->viewerState->currentPosition.z =
            state->skeletonState->activeNode->position.z /
            state->magnification;

        updatePosition(state, TELL_COORDINATE_CHANGE);
    }
}


static void UI_linkActiveNodeWithBtnPressed() {
    if((state->skeletonState->activeNode)
        && findNodeByNodeID(state->viewerState->ag->linkActiveWithNode, state))

        addSegment(CHANGE_MANUAL,
                    state->skeletonState->activeNode->nodeID,
                    state->viewerState->ag->linkActiveWithNode,
                    state);
}

static void UI_actNodeRadiusWdgtModified() {
    if(state->skeletonState->activeNode) {
        editNode(CHANGE_MANUAL,
                 0,
                 state->skeletonState->activeNode,
                 state->viewerState->ag->actNodeRadius,
                 state->skeletonState->activeNode->position.x,
                 state->skeletonState->activeNode->position.y,
                 state->skeletonState->activeNode->position.z,
                 state->magnification,
                 state);
    }
}

static void UI_deleteTreeBtnPressed() {
    yesNoPrompt(NULL, "Do you really want to delete the active tree?", UI_helpDeleteTree, NULL);
}

static void UI_helpDeleteTree(){
    delActiveTree(state);
}

static void UI_newTreeBtnPressed() {
    color4F treeCol;
    /* -1 causes new color assignment */
    treeCol.r = -1.;
    treeCol.g = -1.;
    treeCol.b = -1.;
    treeCol.a = 1.;
    addTreeListElement(TRUE, CHANGE_MANUAL, 0, treeCol);
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
}

static void UI_splitTreeBtnPressed() {
    if(state->skeletonState->activeNode)
        splitConnectedComponent(CHANGE_MANUAL,
                                state->skeletonState->activeNode->nodeID,
                                state);
}

static void UI_mergeTreesBtnPressed() {
    mergeTrees(CHANGE_MANUAL,
                state->viewerState->ag->mergeTreesID1,
                state->viewerState->ag->mergeTreesID2, state);
}


static void UI_renderModelRadioModified() {

    /* true, if lines and points is disabled */
    if(state->viewerState->ag->radioRenderingModel) {
        state->skeletonState->displayMode &= ~DSP_LINES_POINTS;
    }
    else {
        state->skeletonState->displayMode |= DSP_LINES_POINTS;
    }

    state->skeletonState->skeletonChanged = TRUE;
}

static void UI_setHighlightActiveTree() {
    if(state->skeletonState->highlightActiveTree)
        state->skeletonState->highlightActiveTree = FALSE;
    else
        state->skeletonState->highlightActiveTree = TRUE;

    state->skeletonState->skeletonChanged = TRUE;
}

static void UI_setShowVPLabels() {
    if(state->viewerState->showVPLabels){
        state->viewerState->showVPLabels = FALSE;
        AG_WidgetHide(state->viewerState->ag->dataSizeLabelxy);
        AG_WidgetHide(state->viewerState->ag->dataSizeLabelxz);
        AG_WidgetHide(state->viewerState->ag->dataSizeLabelyz);
    }
    else{
        state->viewerState->showVPLabels = TRUE;
        AG_WidgetShow(state->viewerState->ag->dataSizeLabelxy);
        AG_WidgetShow(state->viewerState->ag->dataSizeLabelxz);
        AG_WidgetShow(state->viewerState->ag->dataSizeLabelyz);
    }
}

static void UI_setShowNodeIDs() {
    if(state->skeletonState->showNodeIDs)
        state->skeletonState->showNodeIDs = FALSE;
    else
        state->skeletonState->showNodeIDs = TRUE;

    state->skeletonState->skeletonChanged = TRUE;

}

static void UI_skeletonChanged() {
    state->skeletonState->skeletonChanged = TRUE;

}

static void UI_displayModeRadioModified() {
    /* set the appropriate flag */

    /* reset flags first */
    state->skeletonState->displayMode &=
        (~DSP_SKEL_VP_WHOLE &
        ~DSP_ACTIVETREE &
        ~DSP_SKEL_VP_HIDE &
        ~DSP_SKEL_VP_CURRENTCUBE);

    switch(state->viewerState->ag->radioSkeletonDisplayMode) {
        case 0: /* Whole Skeleton */
            state->skeletonState->displayMode |= DSP_SKEL_VP_WHOLE;
            break;
        case 1: /* Only Current Cube */
            state->skeletonState->displayMode |= DSP_SKEL_VP_CURRENTCUBE;
            break;
        case 2: /* Only Active Tree */
            state->skeletonState->displayMode |= DSP_ACTIVETREE;
            break;
        case 3: /* Hide Skeleton */
            state->skeletonState->displayMode |= DSP_SKEL_VP_HIDE;
            break;
    }

    state->skeletonState->skeletonChanged = TRUE;
}

static void UI_enableSliceVPOverlayModified(AG_Event *event) {
    int overlayOn = AG_INT(1);
    if(!overlayOn) {
        state->skeletonState->displayMode |= DSP_SLICE_VP_HIDE;
    }
    else {
        state->skeletonState->displayMode &= ~DSP_SLICE_VP_HIDE;
    }

    state->skeletonState->skeletonChanged = TRUE;
}

static void UI_pushBranchBtnPressed() {
    SDL_Event ev;

    ev.type = SDL_USEREVENT;
    LOG("pushing something.");
    if(SDL_PushEvent(&ev) == -1) {
        LOG("pushevent returned -1");
    }

    pushBranchNode(CHANGE_MANUAL, TRUE, TRUE, state->skeletonState->activeNode, 0, state);

}
static void UI_popBranchBtnPressed() {
    UI_popBranchNode(CHANGE_MANUAL, state);
}

static void UI_enableLinearFilteringModified(AG_Event *event) {
    int linOn = AG_INT(1);
    switch(linOn) {
        case TRUE:
            tempConfig->viewerState->filterType = GL_LINEAR;
            break;
        case FALSE:
            tempConfig->viewerState->filterType = GL_NEAREST;
            break;
    }

}
static void treeColorAdjustmentsChanged() {
    if(state->viewerState->treeColortableOn) {
        if(state->viewerState->treeLutSet) {
            memcpy(state->viewerState->treeAdjustmentTable,
            state->viewerState->treeColortable,
            state->viewerState->treeLutSize * sizeof(float));
            updateTreeColors();
        }
        else {
            memcpy(state->viewerState->treeAdjustmentTable,
            state->viewerState->neutralTreeTable,
            state->viewerState->treeLutSize * sizeof(float));
        }
    }
    else {
        if(memcmp(state->viewerState->treeAdjustmentTable,
            state->viewerState->neutralTreeTable,
            state->viewerState->treeLutSize * sizeof(float)) != 0) {
            memcpy(state->viewerState->treeAdjustmentTable,
                state->viewerState->neutralTreeTable,
                state->viewerState->treeLutSize * sizeof(float));
                updateTreeColors();
        }
    }
}

static void datasetColorAdjustmentsChanged() {
    int32_t doAdjust = FALSE;
    int32_t i = 0;
    int32_t dynIndex;
    GLuint tempTable[3][256];

    if(state->viewerState->datasetColortableOn) {
        memcpy(state->viewerState->datasetAdjustmentTable,
               state->viewerState->datasetColortable,
               768 * sizeof(GLuint));
        doAdjust = TRUE;
    }
    else {
        memcpy(state->viewerState->datasetAdjustmentTable,
               state->viewerState->neutralDatasetTable,
               768 * sizeof(GLuint));
    }

    /*
     * Apply the dynamic range settings to the adjustment table
     *
     */

    if((state->viewerState->luminanceBias != 0) ||
       (state->viewerState->luminanceRangeDelta != 255)) {
        for(i = 0; i < 256; i++) {
            dynIndex = (int32_t)((i - state->viewerState->luminanceBias) /
                                 (state->viewerState->luminanceRangeDelta / 255.));

            if(dynIndex < 0)
                dynIndex = 0;
            if(dynIndex > 255)
                dynIndex = 255;

            tempTable[0][i] = state->viewerState->datasetAdjustmentTable[0][dynIndex];
            tempTable[1][i] = state->viewerState->datasetAdjustmentTable[1][dynIndex];
            tempTable[2][i] = state->viewerState->datasetAdjustmentTable[2][dynIndex];
        }

        for(i = 0; i < 256; i++) {
            state->viewerState->datasetAdjustmentTable[0][i] = tempTable[0][i];
            state->viewerState->datasetAdjustmentTable[1][i] = tempTable[1][i];
            state->viewerState->datasetAdjustmentTable[2][i] = tempTable[2][i];
        }

        doAdjust = TRUE;
    }

    state->viewerState->datasetAdjustmentOn = doAdjust;
}

static void UI_helpShowAbout() {
	AG_WindowShow(state->viewerState->ag->aboutWin);
}

/* we use undocumented Agar features here... hopefully, vedge won't change them ;-). */
void setGUIcolors() {
    AG_ColorsSetRGB(BG_COLOR, 190, 190, 190);
    AG_ColorsSetRGB(FRAME_COLOR, 150, 150, 150);
    AG_ColorsSetRGB(WINDOW_BG_COLOR, 190, 190, 190);
    AG_ColorsSetRGB(TEXT_COLOR, 0, 0, 0);
    AG_ColorsSetRGB(TITLEBAR_FOCUSED_COLOR,  140, 140, 140);
    AG_ColorsSetRGB(TITLEBAR_UNFOCUSED_COLOR, 110, 110, 110);
    AG_ColorsSetRGB(TITLEBAR_CAPTION_COLOR, 0, 0, 0);
    AG_ColorsSetRGB(TEXTBOX_COLOR, 255, 255, 255);
    AG_ColorsSetRGB(TEXTBOX_TXT_COLOR, 0, 0, 0);
    AG_ColorsSetRGB(TEXTBOX_CURSOR_COLOR, 0, 0, 0);

    AG_ColorsSetRGB(MENU_TXT_COLOR, 0, 0, 0);
    AG_ColorsSetRGB(MENU_UNSEL_COLOR, 200, 200, 200);
    AG_ColorsSetRGB(MENU_SEL_COLOR, 120, 120, 120);
    AG_ColorsSetRGB(LINE_COLOR, 50, 50, 50);
    AG_ColorsSetRGB(BUTTON_COLOR, 160, 160, 160);
    AG_ColorsSetRGB(TABLE_COLOR, 120, 120, 120);
    AG_ColorsSetRGB(TABLE_LINE_COLOR, 50, 50, 50);
    AG_ColorsSetRGB(PANE_COLOR, 190, 190, 190);
    AG_ColorsSetRGB(WINDOW_HI_COLOR, 210, 210, 210);
    AG_ColorsSetRGB(WINDOW_LO_COLOR, 140, 140, 140);
    AG_ColorsSetRGB(NOTEBOOK_TXT_COLOR, 0, 0, 0);
    AG_ColorsSetRGB(NOTEBOOK_BG_COLOR, 170, 170, 170);
    AG_ColorsSetRGB(NOTEBOOK_SEL_COLOR, 210, 210, 210);
    AG_ColorsSetRGB(WINDOW_BORDER_COLOR, 70, 70, 70);
    AG_ColorsSetRGB(FIXED_BG_COLOR, 120, 160, 160);
    AG_ColorsSetRGB(FIXED_BOX_COLOR, 160, 160, 160);
    AG_ColorsSetRGB(TLIST_TXT_COLOR, 0, 0, 0);
    AG_ColorsSetRGB(TLIST_BG_COLOR, 210, 210, 210);
    AG_ColorsSetRGB(TLIST_LINE_COLOR, 190, 190, 190);
    AG_ColorsSetRGB(TLIST_SEL_COLOR, 120, 120, 120);

    AG_ColorsSetRGB(RADIO_TXT_COLOR, 0, 0, 0);
    AG_ColorsSetRGB(RADIO_SEL_COLOR, 50, 50, 50);
    AG_ColorsSetRGB(RADIO_HI_COLOR, 190, 190, 190);
    AG_ColorsSetRGB(RADIO_LO_COLOR, 120, 120, 120);
    AG_ColorsSetRGB(RADIO_OVER_COLOR, 255, 255, 255);

    AG_ColorsSetRGB(SCROLLBAR_COLOR, 0, 0, 0);
    AG_ColorsSetRGB(SCROLLBAR_BTN_COLOR, 160, 160, 160);
    AG_ColorsSetRGB(SCROLLBAR_COLOR, 120, 120, 120);
    AG_ColorsSetRGB(RADIO_OVER_COLOR, 255, 255, 255);

/*
	BG_COLOR,
	FRAME_COLOR,
	LINE_COLOR,
	TEXT_COLOR,
	WINDOW_BG_COLOR,
	WINDOW_HI_COLOR,
	WINDOW_LO_COLOR,
	TITLEBAR_FOCUSED_COLOR,
	TITLEBAR_UNFOCUSED_COLOR,
	TITLEBAR_CAPTION_COLOR,
	BUTTON_COLOR,
	BUTTON_TXT_COLOR,
	DISABLED_COLOR,
	CHECKBOX_COLOR,
	CHECKBOX_TXT_COLOR,
	GRAPH_BG_COLOR,
	GRAPH_XAXIS_COLOR,
	HSVPAL_CIRCLE_COLOR,
	HSVPAL_TILE1_COLOR,
	HSVPAL_TILE2_COLOR,
	MENU_UNSEL_COLOR,
	MENU_SEL_COLOR,
	MENU_OPTION_COLOR,
	MENU_TXT_COLOR,
	MENU_SEP1_COLOR,
	MENU_SEP2_COLOR,
	NOTEBOOK_BG_COLOR,
	NOTEBOOK_SEL_COLOR,
	NOTEBOOK_TXT_COLOR,
	RADIO_SEL_COLOR,
	RADIO_OVER_COLOR,
	RADIO_HI_COLOR,
	RADIO_LO_COLOR,
	RADIO_TXT_COLOR,
	SCROLLBAR_COLOR,
	SCROLLBAR_BTN_COLOR,
	SCROLLBAR_ARR1_COLOR,
	SCROLLBAR_ARR2_COLOR,
	SEPARATOR_LINE1_COLOR,
	SEPARATOR_LINE2_COLOR,
	TABLEVIEW_COLOR,
	TABLEVIEW_HEAD_COLOR,
	TABLEVIEW_HTXT_COLOR,
	TABLEVIEW_CTXT_COLOR,
	TABLEVIEW_LINE_COLOR,
	TABLEVIEW_SEL_COLOR,
	TEXTBOX_COLOR,
	TEXTBOX_TXT_COLOR,
	TEXTBOX_CURSOR_COLOR,
	TLIST_TXT_COLOR,
	TLIST_BG_COLOR,
	TLIST_LINE_COLOR,
	TLIST_SEL_COLOR,
	MAPVIEW_GRID_COLOR,
	MAPVIEW_CURSOR_COLOR,
	MAPVIEW_TILE1_COLOR,
	MAPVIEW_TILE2_COLOR,
	MAPVIEW_MSEL_COLOR,
	MAPVIEW_ESEL_COLOR,
	TILEVIEW_TILE1_COLOR,
	TILEVIEW_TILE2_COLOR,
	TILEVIEW_TEXTBG_COLOR,
	TILEVIEW_TEXT_COLOR,
	TRANSPARENT_COLOR,
	HSVPAL_BAR1_COLOR,
	HSVPAL_BAR2_COLOR,
	PANE_COLOR,
	PANE_CIRCLE_COLOR,
	MAPVIEW_RSEL_COLOR,
	MAPVIEW_ORIGIN_COLOR,
	FOCUS_COLOR,
	TABLE_COLOR,
	TABLE_LINE_COLOR,
	FIXED_BG_COLOR,
	FIXED_BOX_COLOR,
	TEXT_DISABLED_COLOR,
	MENU_TXT_DISABLED_COLOR,
	SOCKET_COLOR,
	SOCKET_LABEL_COLOR,
	SOCKET_HIGHLIGHT_COLOR,
	PROGRESS_BAR_COLOR,
	WINDOW_BORDER_COLOR,
	LAST_COLOR
*/
    return;
}


void UI_saveSettings() {

/* list of missing functionality
remote host
remote port
*/
    xmlChar attrString[1024];
    xmlDocPtr xmlDocument;
    xmlNodePtr settingsXMLNode, currentXMLNode, currentRecentFile, currentWindow;
    char windowType[2048];
    AG_Window *win = NULL;
    memset(attrString, '\0', 1024 * sizeof(xmlChar));
    int32_t i = 0;

    xmlDocument = xmlNewDoc(BAD_CAST"1.0");
    settingsXMLNode = xmlNewDocNode(xmlDocument, NULL, BAD_CAST"KNOSSOS_GUI_settings", NULL);
    xmlDocSetRootElement(xmlDocument, settingsXMLNode);

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"recentFiles", NULL);
    {
        for(i = 0; i < MAX_RECENT_FILES; i++) {
            if(strcmp(state->viewerState->ag->recentFiles[i], "")) {
                currentRecentFile = xmlNewTextChild(currentXMLNode, NULL, BAD_CAST"recentFile", NULL);

                memset(attrString, '\0', 1024);
                xmlStrPrintf(attrString, 1024, BAD_CAST"%d", i + 1);
                xmlNewProp(currentRecentFile, BAD_CAST"pos", attrString);

                memset(attrString, '\0', 1024);
                xmlStrPrintf(attrString, 1024, BAD_CAST"%s", state->viewerState->ag->recentFiles[i]);
                xmlNewProp(currentRecentFile, BAD_CAST"path", attrString);
            }
        }
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"windowSize", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->screenSizeX);
        xmlNewProp(currentXMLNode, BAD_CAST"screenSizeX", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->screenSizeY);
        xmlNewProp(currentXMLNode, BAD_CAST"screenSizeY", attrString);
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"windows", NULL);
    {
        for(i = 0; i <= 6; i++) {
            switch(i) {
                case 0:
                    win = state->viewerState->ag->toolsWin;
                    strcpy(windowType, "tools");
                    break;
                case 1:
                    win = state->viewerState->ag->zoomingWin;
                    strcpy(windowType, "zoom");
                    break;
                case 2:
                    win = state->viewerState->ag->navOptWin;
                    strcpy(windowType, "nav-options");
                    break;
                case 3:
                    win = state->viewerState->ag->syncOptWin;
                    strcpy(windowType, "sync-options");
                    break;
                case 4:
                    win = state->viewerState->ag->viewPortPrefWin;
                    strcpy(windowType, "vp-options");
                    break;
                case 5:
                    win = state->viewerState->ag->saveOptWin;
                    strcpy(windowType, "saving-options");
                    break;
                case 6:
                    win = state->viewerState->ag->consoleWin;
                    strcpy(windowType, "console");
                    break;
            }

            currentWindow = xmlNewTextChild(currentXMLNode, NULL, BAD_CAST"window", NULL);

            memset(attrString, '\0', 1024);
            xmlStrPrintf(attrString, 1024, BAD_CAST"%s", windowType);
            xmlNewProp(currentWindow, BAD_CAST"type", attrString);

            memset(attrString, '\0', 1024);
            xmlStrPrintf(attrString, 1024, BAD_CAST"%d", ((AG_Widget *)win)->x);
            xmlNewProp(currentWindow, BAD_CAST"x", attrString);

            memset(attrString, '\0', 1024);
            xmlStrPrintf(attrString, 1024, BAD_CAST"%d", ((AG_Widget *)win)->y);
            xmlNewProp(currentWindow, BAD_CAST"y", attrString);

            memset(attrString, '\0', 1024);
            xmlStrPrintf(attrString, 1024, BAD_CAST"%d", ((AG_Widget *)win)->w);
            xmlNewProp(currentWindow, BAD_CAST"w", attrString);

            memset(attrString, '\0', 1024);
            xmlStrPrintf(attrString, 1024, BAD_CAST"%d", ((AG_Widget *)win)->h);
            xmlNewProp(currentWindow, BAD_CAST"h", attrString);

            memset(attrString, '\0', 1024);
            xmlStrPrintf(attrString, 1024, BAD_CAST"%d", win->visible);
            xmlNewProp(currentWindow, BAD_CAST"visible", attrString);
        }
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"workModes", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->workMode);
        xmlNewProp(currentXMLNode, BAD_CAST"skeletonWorkMode", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->workMode);
        xmlNewProp(currentXMLNode, BAD_CAST"viewWorkMode", attrString);
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"datasetNavigation", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->recenteringTime);
        xmlNewProp(currentXMLNode, BAD_CAST"recenteringTime", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->stepsPerSec);
        xmlNewProp(currentXMLNode, BAD_CAST"movementSpeed", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->dropFrames);
        xmlNewProp(currentXMLNode, BAD_CAST"jumpFrames", attrString);
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"dataSavingOptions", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->autoSaveBool);
        xmlNewProp(currentXMLNode, BAD_CAST"autoSaving", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->autoSaveInterval);
        xmlNewProp(currentXMLNode, BAD_CAST"savingInterval", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->autoFilenameIncrementBool);
        xmlNewProp(currentXMLNode, BAD_CAST"autoIncrFileName", attrString);
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"vpSettingsGeneral", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->lightOnOff);
        xmlNewProp(currentXMLNode, BAD_CAST"lightEffects", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->highlightActiveTree);
        xmlNewProp(currentXMLNode, BAD_CAST"highlActiveTree", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->showNodeIDs);
        xmlNewProp(currentXMLNode, BAD_CAST"showAllNodeIDs", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->overrideNodeRadiusBool);
        xmlNewProp(currentXMLNode, BAD_CAST"enableOverrideNodeRadius", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%f", state->skeletonState->overrideNodeRadiusVal);
        xmlNewProp(currentXMLNode, BAD_CAST"overrideNodeRadius", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%f", state->skeletonState->segRadiusToNodeRadius);
        xmlNewProp(currentXMLNode, BAD_CAST"edgeNodeRadiusRatio", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->displayMode);
        xmlNewProp(currentXMLNode, BAD_CAST"displayModeBitFlags", attrString);
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"vpSettingsSliceVPs", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->drawVPCrosshairs);
        xmlNewProp(currentXMLNode, BAD_CAST"vpCrosshairs", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->showIntersections);
        xmlNewProp(currentXMLNode, BAD_CAST"highlIntersections", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%f", state->viewerState->depthCutOff);
        xmlNewProp(currentXMLNode, BAD_CAST"depthCutoff", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->filterType);
        xmlNewProp(currentXMLNode, BAD_CAST"datasetLinearFiltering", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->luminanceBias);
        xmlNewProp(currentXMLNode, BAD_CAST"dynRangeBias", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->luminanceRangeDelta);
        xmlNewProp(currentXMLNode, BAD_CAST"dynRangeDelta", attrString);
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"vpSettingsZoom", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%f", state->viewerState->viewPorts[VIEWPORT_XY].texture.zoomLevel);
        xmlNewProp(currentXMLNode, BAD_CAST"XYPlane", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%f", state->viewerState->viewPorts[VIEWPORT_XZ].texture.zoomLevel);
        xmlNewProp(currentXMLNode, BAD_CAST"XZPlane", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%f", state->viewerState->viewPorts[VIEWPORT_YZ].texture.zoomLevel);
        xmlNewProp(currentXMLNode, BAD_CAST"YZPlane", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%f", state->skeletonState->zoomLevel);
        xmlNewProp(currentXMLNode, BAD_CAST"SkelVP", attrString);
    }
    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"vpSettingsSkelVP", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->showXYplane);
        xmlNewProp(currentXMLNode, BAD_CAST"showXY", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->showXZplane);
        xmlNewProp(currentXMLNode, BAD_CAST"showXZ", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->showYZplane);
        xmlNewProp(currentXMLNode, BAD_CAST"showYZ", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->rotateAroundActiveNode);
        xmlNewProp(currentXMLNode, BAD_CAST"rotateAroundActNode", attrString);
    }

    currentXMLNode = xmlNewTextChild(settingsXMLNode, NULL, BAD_CAST"toolsNodesTab", NULL);
    {
        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->viewerState->ag->useLastActNodeRadiusAsDefault);
        xmlNewProp(currentXMLNode, BAD_CAST"useLastRadiusAsDefault", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%f", state->skeletonState->defaultNodeRadius);
        xmlNewProp(currentXMLNode, BAD_CAST"defaultRadius", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d",  state->skeletonState->lockPositions);
        xmlNewProp(currentXMLNode, BAD_CAST"enableCommentLocking", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%d", state->skeletonState->lockRadius);
        xmlNewProp(currentXMLNode, BAD_CAST"lockingRadius", attrString);

        memset(attrString, '\0', 1024);
        xmlStrPrintf(attrString, 1024, BAD_CAST"%s", state->skeletonState->onCommentLock);
        xmlNewProp(currentXMLNode, BAD_CAST"lockToNodesWithComment", attrString);
    }

    LOG("saved settings to %s\n", state->viewerState->ag->settingsFile);
    xmlSaveFormatFile(state->viewerState->ag->settingsFile, xmlDocument, 1);

    xmlFreeDoc(xmlDocument);
}


static void UI_loadSettings() {
    xmlDocPtr xmlDocument;
    xmlNodePtr currentXMLNode, currentRecentFile, currentWindow;
    xmlChar *attribute, *path, *pos;
    char *win;
    int32_t reqScreenSizeX = 0, reqScreenSizeY = 0, x_win = 0, y_win = 0, w_win = 0, h_win = 0, visible_win = 0;
    AG_Window *win_ptr = NULL;

    /*
     *  There's no sanity check for values read from files, yet.
     */

    xmlDocument = xmlParseFile(state->viewerState->ag->settingsFile);

    if(xmlDocument == NULL) {
        LOG("Document not parsed successfully.");
        return;
    }

    currentXMLNode = xmlDocGetRootElement(xmlDocument);
    if(currentXMLNode == NULL) {
        LOG("Empty settings document. Not loading.");
        xmlFreeDoc(xmlDocument);
        return;
    }

    if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"KNOSSOS_GUI_settings") == FALSE) {
        LOG("Root element %s in file %s unrecognized. Not loading.",
            currentXMLNode->name,
            state->viewerState->ag->settingsFile);
        return;
    }

    currentXMLNode = currentXMLNode->xmlChildrenNode;
    while(currentXMLNode) {
        if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"recentFiles")) {
            currentRecentFile = currentXMLNode->xmlChildrenNode;
            while(currentRecentFile) {
                if(xmlStrEqual(currentRecentFile->name, (const xmlChar *)"recentFile")) {
                    path = xmlGetProp(currentRecentFile, (const xmlChar *)"path");
                    pos = xmlGetProp(currentRecentFile, (const xmlChar *)"pos");

                    if(path) {
                        if(pos) {
                            addRecentFile((char *)path, atoi((char *)pos));
                        }
                        else {
                            addRecentFile((char *)path, 0);
                        }
                    }
                }

                currentRecentFile = currentRecentFile->next;
            }
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"windowSize")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"screenSizeX");
            if(attribute)
                reqScreenSizeX = atoi((char *)attribute);

            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"screenSizeY");
            if(attribute)
                reqScreenSizeY = atoi((char *)attribute);

            if(reqScreenSizeX > 0 && reqScreenSizeY > 0) {
                AG_ResizeDisplay(reqScreenSizeX, reqScreenSizeY);
            }
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"windows")) {
            currentWindow = currentXMLNode->xmlChildrenNode;
            while(currentWindow) {
                if(xmlStrEqual(currentWindow->name, (const xmlChar *)"window")) {
                    attribute = xmlGetProp(currentWindow, (const xmlChar *)"type");
                    if(attribute)
                        win = (char *)attribute;

                    attribute = xmlGetProp(currentWindow, (const xmlChar *)"x");
                    if(attribute)
                        x_win = atoi((char *)attribute);

                    attribute = xmlGetProp(currentWindow, (const xmlChar *)"y");
                    if(attribute)
                        y_win = atoi((char *)attribute);

                    attribute = xmlGetProp(currentWindow, (const xmlChar *)"w");
                    if(attribute)
                        w_win = atoi((char *)attribute);

                    attribute = xmlGetProp(currentWindow, (const xmlChar *)"h");
                    if(attribute)
                        h_win = atoi((char *)attribute);

                    attribute = xmlGetProp(currentWindow, (const xmlChar *)"visible");
                    if(attribute)
                        visible_win = atoi((char *)attribute);

                    if(win && x_win && y_win) {
                        /* Open and position window. */
                        if(strcmp(win, "tools") == 0) {
                            win_ptr = state->viewerState->ag->toolsWin;
                        }
                        if(strcmp(win, "zoom") == 0) {
                            win_ptr = state->viewerState->ag->zoomingWin;
                        }
                        if(strcmp(win, "nav-options") == 0) {
                            win_ptr = state->viewerState->ag->navOptWin;
                        }
                        if(strcmp(win, "disp-options") == 0) {
                            win_ptr = state->viewerState->ag->dispOptWin;
                        }
                        if(strcmp(win, "sync-options") == 0) {
                            win_ptr = state->viewerState->ag->syncOptWin;
                        }
                        if(strcmp(win, "render-options") == 0) {
                            win_ptr = state->viewerState->ag->renderingOptWin;
                        }
                        if(strcmp(win, "spatial-lock-options") == 0) {
                            win_ptr = state->viewerState->ag->spatLockOptWin;
                        }
                        if(strcmp(win, "volume-tracing-options") == 0) {
                            win_ptr = state->viewerState->ag->volTraceOptWin;
                        }
                        if(strcmp(win, "vp-options") == 0) {
                            win_ptr = state->viewerState->ag->viewPortPrefWin;
                        }
                        if(strcmp(win, "saving-options") == 0) {
                            win_ptr = state->viewerState->ag->saveOptWin;
                        }
                        if(strcmp(win, "dynrange-options") == 0) {
                            win_ptr = state->viewerState->ag->setDynRangeWin;
                        }
                        if(strcmp(win, "console") == 0) {
                            win_ptr = state->viewerState->ag->consoleWin;
                        }

                        if(win_ptr) {
                            if(visible_win) {
                                AG_WindowShow(win_ptr);
                                AG_WindowSetGeometry(win_ptr, x_win, y_win, w_win, h_win);
                            }
                            else {
                                AG_WindowHide(win_ptr);
                            }
                        }
                    }
                }

                currentWindow = currentWindow->next;

            }
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"workModes")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"skeletonWorkMode");
            if(attribute)
                state->skeletonState->workMode = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"viewWorkMode");
            if(attribute)
                tempConfig->viewerState->workMode = atoi((char *)attribute);
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"datasetNavigation")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"recenteringTime");
            if(attribute)
                state->viewerState->recenteringTime = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"movementSpeed");
            if(attribute)
                tempConfig->viewerState->stepsPerSec = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"jumpFrames");
            if(attribute)
                tempConfig->viewerState->dropFrames = atoi((char *)attribute);
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"dataSavingOptions")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"autoSaving");
            if(attribute)
                state->skeletonState->autoSaveBool = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"savingInterval");
            if(attribute)
                state->skeletonState->autoSaveInterval = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"autoIncrFileName");
            if(attribute)
                state->skeletonState->autoFilenameIncrementBool = atoi((char *)attribute);
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"vpSettingsGeneral")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"lightEffects");
            if(attribute)
                state->viewerState->lightOnOff = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"highlActiveTree");
            if(attribute)
                state->skeletonState->highlightActiveTree = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"showAllNodeIDs");
            if(attribute)
                state->skeletonState->showNodeIDs = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"enableOverrideNodeRadius");
            if(attribute)
                state->skeletonState->overrideNodeRadiusBool = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"overrideNodeRadius");
            if(attribute)
                state->skeletonState->overrideNodeRadiusVal = atof((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"edgeNodeRadiusRatio");
            if(attribute)
                state->skeletonState->segRadiusToNodeRadius = atof((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"displayModeBitFlags");
            if(attribute)
                state->skeletonState->displayMode = atoi((char *)attribute);
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"vpSettingsSliceVPs")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"highlIntersections");
            if(attribute)
                state->skeletonState->showIntersections = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"depthCutoff");
            if(attribute)
                state->viewerState->depthCutOff = atof((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"datasetLinearFiltering");
            if(attribute)
                state->viewerState->filterType = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"dynRangeBias");
            if(attribute)
                state->viewerState->luminanceBias = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"dynRangeDelta");
            if(attribute)
                state->viewerState->luminanceRangeDelta = atoi((char *)attribute);
        }

        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *) "vpSettingsZoom")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *) "XYPlane");
            float zoomlevel = atof ((char *) attribute);
            if(0.2 <= zoomlevel && zoomlevel <= 1.) {
                state->viewerState->viewPorts[VIEWPORT_XY].texture.zoomLevel = zoomlevel;
            }
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *) "XZPlane");
            zoomlevel = atof ((char *) attribute);
            if(0.2 <= zoomlevel && zoomlevel <= 1.) {
                state->viewerState->viewPorts[VIEWPORT_XZ].texture.zoomLevel = zoomlevel;
            }
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *) "YZPlane");
            zoomlevel = atof ((char *) attribute);
            if(0.2 <= zoomlevel && zoomlevel <= 1.) {
                state->viewerState->viewPorts[VIEWPORT_YZ].texture.zoomLevel = zoomlevel;
            }
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *) "SkelVP");
            zoomlevel = atof ((char *) attribute);
            if(0.2 <= zoomlevel && zoomlevel <= 1.) {
                state->skeletonState->zoomLevel = zoomlevel;
            }
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"vpSettingsSkelVP")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"showXY");
            if(attribute)
                state->skeletonState->showXYplane = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"showXZ");
            if(attribute)
                state->skeletonState->showXZplane = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"showYZ");
            if(attribute)
                state->skeletonState->showYZplane = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"rotateAroundActNode");
            if(attribute)
                state->skeletonState->rotateAroundActiveNode = atoi((char *)attribute);
        }
        else if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"toolsNodesTab")) {
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"useLastRadiusAsDefault");
            if(attribute)
                state->viewerState->ag->useLastActNodeRadiusAsDefault = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"defaultRadius");
            if(attribute)
                state->skeletonState->defaultNodeRadius = atof((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"enableCommentLocking");
            if(attribute)
                state->skeletonState->lockPositions = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"lockingRadius");
            if(attribute)
                state->skeletonState->lockRadius = atoi((char *)attribute);
            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"lockToNodesWithComment");
            if(attribute)
				strcpy(state->skeletonState->onCommentLock, (char *)attribute);
        }

        currentXMLNode = currentXMLNode->next;
    }

    xmlFreeDoc(xmlDocument);
    state->viewerState->ag->enableOrthoSkelOverlay = 1;
    if(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE) {
        state->viewerState->ag->enableOrthoSkelOverlay = 0;
    }
    if(state->skeletonState->displayMode & DSP_SKEL_VP_WHOLE)
        state->viewerState->ag->radioSkeletonDisplayMode = 0;
    if(state->skeletonState->displayMode & DSP_SKEL_VP_CURRENTCUBE)
        state->viewerState->ag->radioSkeletonDisplayMode = 1;
    if(state->skeletonState->displayMode & DSP_ACTIVETREE)
        state->viewerState->ag->radioSkeletonDisplayMode = 2;
    if(state->skeletonState->displayMode & DSP_SKEL_VP_HIDE)
        state->viewerState->ag->radioSkeletonDisplayMode = 3;
    if(state->skeletonState->displayMode & DSP_LINES_POINTS)
        state->viewerState->ag->radioRenderingModel = 0;

    LOG("loaded GUI settings from file %s.", state->viewerState->ag->settingsFile);
}

void UI_loadSkeleton(AG_Event *event) {
    char *path = AG_STRING(1);
    char *msg = AG_STRING(2);

    strncpy(state->skeletonState->skeletonFile, path, 8192);

    if(state->skeletonState->totalNodeElements != 0) {
        yesNoPrompt(NULL, msg, WRAP_loadSkeleton, NULL);
    }
    else {
        WRAP_loadSkeleton();
    }
}

static void WRAP_loadSkeleton() {
    if(loadSkeleton()) {
        updateTitlebar(TRUE);
        LOG("Successfully loaded %s", state->skeletonState->skeletonFile);
        UI_workModeLink();
    }
    else {
        AG_TextError("The skeleton was not loaded successfully.\n"
                     "Check the log console for more details."
                     "Attempted to load %s", state->skeletonState->skeletonFile);
        LOG("Error loading %s", state->skeletonState->skeletonFile);
    }
}

static void updateTitlebar(int32_t useFilename) {
    char *filename;

#ifdef LINUX
    filename = strrchr(state->skeletonState->skeletonFile, '/');
#else
    filename = strrchr(state->skeletonState->skeletonFile, '\\');
#endif

    if(filename) {
        filename++;
    }
    else {
        filename = state->viewerState->ag->titleString;
    }

    if(useFilename) {
        snprintf(state->viewerState->ag->titleString, 2047, "KNOSSOS %s showing %s [%s]", KVERSION, state->name, filename);
    }
    else {
        snprintf(state->viewerState->ag->titleString, 2047, "KNOSSOS %s showing %s [%s]", KVERSION, state->name, "no skeleton file");
    }

    SDL_WM_SetCaption(state->viewerState->ag->titleString, NULL);
}

void UI_copyClipboardCoordinates() {
    char copyString[8192];

    memset(copyString, '\0', 8192);

    snprintf(copyString,
             8192,
             "%d, %d, %d",
             state->viewerState->currentPosition.x + 1,
             state->viewerState->currentPosition.y + 1,
             state->viewerState->currentPosition.z + 1);

    SDLScrap_CopyToClipboard(SDL_CLIPBOARD_TEXT_TYPE, strlen(copyString), copyString);
}

void UI_pasteClipboardCoordinates() {
    size_t len = 0;
    char *pasteBuffer = NULL;
    Coordinate *extractedCoords = NULL;

    SDLScrap_PasteFromClipboard(SDL_CLIPBOARD_TEXT_TYPE, &len, &pasteBuffer);

    if(len <= 0)
    {
        LOG("Unable to fetch text from clipboard");
        return;
    }

    if((extractedCoords = parseRawCoordinateString(pasteBuffer))) {
        tempConfig->viewerState->currentPosition.x = extractedCoords->x - 1;
        tempConfig->viewerState->currentPosition.y = extractedCoords->y - 1;
        tempConfig->viewerState->currentPosition.z = extractedCoords->z - 1;

        updatePosition(state, TELL_COORDINATE_CHANGE);
        refreshViewports(state);

        free(extractedCoords);
    }
    else {
        // LOG("Unable to extract coordinates from clipboard.");
    }

    SDLScrap_FreeBuffer(pasteBuffer);

    return;
}

static Coordinate *parseRawCoordinateString(char *string) {
    Coordinate *extractedCoords = NULL;
    char str[strlen(string)];
    strcpy(str, string);
    char delims[] = "[]()./,; -";
    char* result = NULL;
    char* coords[3];
    int i = 0;

    if(!(extractedCoords = malloc(sizeof(Coordinate)))) {
        LOG("Out of memory");
        _Exit(FALSE);
    }

    result = strtok(str, delims);
    while( result != NULL ) {
        coords[i] = malloc(strlen(result)+1);
        strcpy(coords[i], result);
        result = strtok( NULL, delims );
        ++i;
    }

	if(i < 2) {
        LOG("Paste string doesn't contain enough delimiter-separated elements");
        goto fail;
    }

    if((extractedCoords->x = atoi(coords[0])) < 1) {
        LOG("Error converting paste string to coordinate");
        goto fail;
    }
    if((extractedCoords->y = atoi(coords[1])) < 1) {
		LOG("Error converting paste string to coordinate");
        goto fail;
    }
    if((extractedCoords->z = atoi(coords[2])) < 1) {
        LOG("Error converting paste string to coordinate");
        goto fail;
    }

    return extractedCoords;

fail:
    free(extractedCoords);
    return NULL;
}

static void UI_setSkeletonPerspective(AG_Event *event) {
    int buttonPressed = AG_INT(1);

    switch(buttonPressed) {
        case 1:
            /* Show XY surface */

            state->skeletonState->definedSkeletonVpView = 1;
            break;
        case 2:
            /* Show XZ surface */
            state->skeletonState->definedSkeletonVpView = 3;

            break;
        case 3:
            /* Show YZ surface */
            state->skeletonState->definedSkeletonVpView = 2;
            break;
        case 4:
            /* Flip */

            state->skeletonState->definedSkeletonVpView = 4;
            break;
        case 5:
            /* Reset */
            state->skeletonState->definedSkeletonVpView = 5;
            break;
    }

    /* TODO WTF */
    drawGUI(state);
    drawGUI(state);
}

uint32_t addRecentFile(char *path, uint32_t pos) {
    /*
     * Add a path to the list of recently opened files.
     * If pos is 0, add it to the first position and move all
     * other paths down one position.
     * If pos is a positive integer, overwrite the string at
     * that position with path.
     *
     */

    int32_t i = 0;
    AG_MenuItem *currentNode;

    if(pos > 0 && pos <= MAX_RECENT_FILES) {
        /* Add to a specific position, do not move other paths. */

        strncpy(state->viewerState->ag->recentFiles[pos - 1], path, 4096);
    }
    else {
        /* Add to top, move other strings down. */

        for(i = MAX_RECENT_FILES - 1; i > 0; i--) {
            strncpy(state->viewerState->ag->recentFiles[i],
                    state->viewerState->ag->recentFiles[i - 1],
                    4096);
        }

        strncpy(state->viewerState->ag->recentFiles[0], path, 4096);
    }

    /* Update menu */

    /* Get Recent Files node. I just can't believe this is necessary. WTF? */
    for (i = 0; i < state->viewerState->ag->appMenuRoot->nsubitems; i++) {
        if(strcmp((&state->viewerState->ag->appMenuRoot->subitems[i])->text,
                  "File") == 0) {
            currentNode = &state->viewerState->ag->appMenuRoot->subitems[i];
            break;
        }
    }

    for (i = 0; i < currentNode->nsubitems; i++) {
        if(strcmp((&currentNode->subitems[i])->text,
                  "Recent Files") == 0) {
            currentNode = &currentNode->subitems[i];
            break;
        }
    }

    /* Delete menu contents */

    AG_MenuItemFreeChildren(currentNode);

    /* Rebuild menu contents */
    for(i = 0; i < MAX_RECENT_FILES; i++) {
        if(strcmp(state->viewerState->ag->recentFiles[i], "")) {
            AG_MenuAction(currentNode,
                          state->viewerState->ag->recentFiles[i],
                          NULL,
                          UI_loadSkeleton,
                          "%s,%s",
                          state->viewerState->ag->recentFiles[i],
                          "This will overwrite the currently loaded skeleton. Are you sure?");
        }
    }

    return TRUE;
}

uint32_t cpBaseDirectory(char *target, char *path, size_t len) {
    char *hit;
    int32_t baseLen;

#ifdef LINUX
    hit = strrchr(path, '/');
#else
    hit = strrchr(path, '\\');
#endif

    if(hit == NULL) {
        LOG("Cannot find a path separator char in %s\n", path);
        return FALSE;
    }

    baseLen = (int32_t)(hit - path);
    if(baseLen > 2047) {
        LOG("Path too long\n");
        return FALSE;
    }

    strncpy(target, path, baseLen);
    target[baseLen] = '\0';

    return TRUE;
}
