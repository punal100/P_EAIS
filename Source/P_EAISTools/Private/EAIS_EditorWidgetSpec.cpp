/*
 * @Author: Punal Manalan
 * @Description: EAIS Editor Widget Spec - Properly styled launcher
 * @Date: 29/12/2025
 * 
 * This creates a properly styled EUW launcher for the EAIS Graph Editor.
 * The EUW uses ONLY P_MWCS-supported widgets.
 */

#include "EAIS_EditorWidgetSpec.h"

FString UEAIS_EditorWidgetSpec::GetWidgetSpec()
{
    // Complete spec with proper styling, button text as children, and layout
    static FString Spec = R"JSON({
    "WidgetClass": "UEAIS_AIEditor",
    "BlueprintName": "EUW_EAIS_AIEditor",
    "ParentClass": "/Script/P_EAISTools.EAIS_AIEditor",
    "Category": "EAIS|Editor|UI",
    "Description": "EAIS AI Editor Launcher",
    "Version": "3.0.0",
    "IsToolEUW": true,

    "DesignerPreview": {
        "SizeMode": "DesiredOnScreen",
        "ZoomLevel": 14,
        "ShowGrid": true
    },
    
    "Hierarchy": {
        "Root": {
            "Type": "CanvasPanel",
            "Name": "RootCanvas",
            "Children": [
                {
                    "Type": "VerticalBox",
                    "Name": "MainContainer",
                    "Slot": {
                        "Anchors": {"Min": {"X": 0, "Y": 0}, "Max": {"X": 1, "Y": 1}},
                        "Offsets": {"Left": 0, "Top": 0, "Right": 0, "Bottom": 0},
                        "Alignment": {"X": 0, "Y": 0}
                    },
                    "Children": [
                        {
                            "Type": "Border",
                            "Name": "HeaderBorder",
                            "Slot": {"Size": {"Rule": "Auto"}, "Padding": {"Left": 0, "Top": 0, "Right": 0, "Bottom": 0}},
                            "BrushColor": {"R": 0.02, "G": 0.02, "B": 0.05, "A": 1},
                            "Padding": {"Left": 16, "Top": 12, "Right": 16, "Bottom": 12},
                            "Children": [
                                {
                                    "Type": "TextBlock",
                                    "Name": "TitleText",
                                    "Text": "EAIS AI Editor",
                                    "FontSize": 20,
                                    "Justification": "Left",
                                    "AutoWrapText": false
                                }
                            ]
                        },
                        {
                            "Type": "Border",
                            "Name": "ContentBorder",
                            "Slot": {"Size": {"Rule": "Fill", "Value": 1}, "Padding": {"Left": 0, "Top": 0, "Right": 0, "Bottom": 0}},
                            "BrushColor": {"R": 0.015, "G": 0.015, "B": 0.02, "A": 1},
                            "Padding": {"Left": 16, "Top": 16, "Right": 16, "Bottom": 16},
                            "Children": [
                                {
                                    "Type": "VerticalBox",
                                    "Name": "ContentVBox",
                                    "Children": [
                                        {
                                            "Type": "TextBlock",
                                            "Text": "Select a profile and click 'Open Graph Editor' to edit AI behaviors visually.",
                                            "FontSize": 11,
                                            "AutoWrapText": true,
                                            "Slot": {"Padding": {"Bottom": 16}}
                                        },
                                        {
                                            "Type": "HorizontalBox",
                                            "Name": "ProfileRow",
                                            "Slot": {"Size": {"Rule": "Auto"}, "Padding": {"Bottom": 8}},
                                            "Children": [
                                                {"Type": "TextBlock", "Text": "Profile:", "FontSize": 12, "Slot": {"VAlign": "Center"}},
                                                {"Type": "ComboBoxString", "Name": "ProfileDropdown", "Slot": {"Size": {"Rule": "Fill", "Value": 1}, "Padding": {"Left": 8, "Right": 8}, "VAlign": "Center"}},
                                                {
                                                    "Type": "Button",
                                                    "Name": "Btn_ListProfiles",
                                                    "Slot": {"Size": {"Rule": "Auto"}},
                                                    "Children": [
                                                        {"Type": "TextBlock", "Text": "Refresh", "FontSize": 11, "Slot": {"HAlign": "Center", "VAlign": "Center", "Padding": {"Left": 12, "Right": 12, "Top": 4, "Bottom": 4}}}
                                                    ]
                                                }
                                            ]
                                        },
                                        {
                                            "Type": "HorizontalBox",
                                            "Name": "SelectedRow",
                                            "Slot": {"Size": {"Rule": "Auto"}, "Padding": {"Bottom": 16}},
                                            "Children": [
                                                {"Type": "TextBlock", "Text": "Selected:", "FontSize": 12, "Slot": {"VAlign": "Center"}},
                                                {"Type": "TextBlock", "Name": "ProfileNameText", "Text": "(none)", "FontSize": 12, "Slot": {"Padding": {"Left": 8}, "VAlign": "Center"}}
                                            ]
                                        },
                                        {
                                            "Type": "Button",
                                            "Name": "Btn_OpenGraphEditor",
                                            "Slot": {"Size": {"Rule": "Auto"}, "Padding": {"Bottom": 12}},
                                            "Children": [
                                                {"Type": "TextBlock", "Text": "Open Graph Editor", "FontSize": 14, "Slot": {"HAlign": "Center", "VAlign": "Center", "Padding": {"Left": 24, "Right": 24, "Top": 8, "Bottom": 8}}}
                                            ]
                                        },
                                        {
                                            "Type": "HorizontalBox",
                                            "Name": "ActionRow",
                                            "Slot": {"Size": {"Rule": "Auto"}, "Padding": {"Bottom": 8}},
                                            "Children": [
                                                {
                                                    "Type": "Button",
                                                    "Name": "Btn_Load",
                                                    "Slot": {"Size": {"Rule": "Fill", "Value": 1}, "Padding": {"Right": 4}},
                                                    "Children": [
                                                        {"Type": "TextBlock", "Text": "Load", "FontSize": 11, "Slot": {"HAlign": "Center", "VAlign": "Center", "Padding": {"Top": 6, "Bottom": 6}}}
                                                    ]
                                                },
                                                {
                                                    "Type": "Button",
                                                    "Name": "Btn_Validate",
                                                    "Slot": {"Size": {"Rule": "Fill", "Value": 1}, "Padding": {"Left": 4, "Right": 4}},
                                                    "Children": [
                                                        {"Type": "TextBlock", "Text": "Validate", "FontSize": 11, "Slot": {"HAlign": "Center", "VAlign": "Center", "Padding": {"Top": 6, "Bottom": 6}}}
                                                    ]
                                                },
                                                {
                                                    "Type": "Button",
                                                    "Name": "Btn_ExportRuntime",
                                                    "Slot": {"Size": {"Rule": "Fill", "Value": 1}, "Padding": {"Left": 4, "Right": 4}},
                                                    "Children": [
                                                        {"Type": "TextBlock", "Text": "Export", "FontSize": 11, "Slot": {"HAlign": "Center", "VAlign": "Center", "Padding": {"Top": 6, "Bottom": 6}}}
                                                    ]
                                                },
                                                {
                                                    "Type": "Button",
                                                    "Name": "Btn_TestSpawn",
                                                    "Slot": {"Size": {"Rule": "Fill", "Value": 1}, "Padding": {"Left": 4}},
                                                    "Children": [
                                                        {"Type": "TextBlock", "Text": "Test Spawn", "FontSize": 11, "Slot": {"HAlign": "Center", "VAlign": "Center", "Padding": {"Top": 6, "Bottom": 6}}}
                                                    ]
                                                }
                                            ]
                                        },
                                        {"Type": "Spacer", "Slot": {"Size": {"Rule": "Fill", "Value": 1}}},
                                        {
                                            "Type": "Border",
                                            "Name": "StatusBorder",
                                            "Slot": {"Size": {"Rule": "Auto"}},
                                            "BrushColor": {"R": 0.01, "G": 0.01, "B": 0.015, "A": 1},
                                            "Padding": {"Left": 8, "Top": 6, "Right": 8, "Bottom": 6},
                                            "Children": [
                                                {"Type": "TextBlock", "Name": "StatusText", "Text": "Ready", "FontSize": 11, "AutoWrapText": true}
                                            ]
                                        }
                                    ]
                                }
                            ]
                        }
                    ]
                }
            ]
        }
    },
    
    "Bindings": {
        "Required": [
            {"Name": "ProfileDropdown", "Type": "UComboBoxString", "Purpose": "Profile selection dropdown"},
            {"Name": "ProfileNameText", "Type": "UTextBlock", "Purpose": "Selected profile name"},
            {"Name": "StatusText", "Type": "UTextBlock", "Purpose": "Status text"},
            {"Name": "Btn_OpenGraphEditor", "Type": "UButton", "Purpose": "Opens graph editor tab"},
            {"Name": "Btn_ListProfiles", "Type": "UButton", "Purpose": "Refresh profiles"},
            {"Name": "Btn_Load", "Type": "UButton", "Purpose": "Load profile"},
            {"Name": "Btn_Validate", "Type": "UButton", "Purpose": "Validate profile"},
            {"Name": "Btn_ExportRuntime", "Type": "UButton", "Purpose": "Export runtime JSON"},
            {"Name": "Btn_TestSpawn", "Type": "UButton", "Purpose": "Test spawn AI"}
        ]
    },
    
    "Design": {
        "TitleText": {"FontSize": 20},
        "ProfileNameText": {"FontSize": 12},
        "StatusText": {"FontSize": 11, "AutoWrapText": true}
    }
})JSON";
    return Spec;
}
