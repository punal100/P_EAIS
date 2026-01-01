/*
 * @Author: Punal Manalan
 * @Description: Implementation of EAIS Editor Widget Spec
 * @Date: 29/12/2025
 */

#include "EAIS_EditorWidgetSpec.h"

FString UEAIS_EditorWidgetSpec::GetWidgetSpec()
{
    // JSON specification for the EAIS Editor widget
    // This will be processed by P_MWCS to generate the actual widget
    // Note: The native UEAIS_AIEditor class already defines the widget bindings,
    // so we keep the spec simple with just layout structure.
    
    static FString Spec = R"JSON({
    "WidgetClass": "UEAIS_AIEditor",
    "BlueprintName": "EUW_EAIS_AIEditor",
    "ParentClass": "/Script/Blutility.EditorUtilityWidget",
    "Category": "EAIS|Editor|UI",
    "Description": "Visual AI Editor for P_EAIS JSON-based AI behaviors",
    "Version": "1.0.0",

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
                        "Offsets": {"Left": 10, "Top": 10, "Right": 10, "Bottom": 10}
                    },
                    "Children": [
                        {
                            "Type": "HorizontalBox",
                            "Name": "ToolbarContainer",
                            "Slot": {"Size": {"Rule": "Auto"}},
                            "Children": [
                                {"Type": "TextBlock", "Name": "TitleText", "Text": "EAIS AI Editor", "FontSize": 18},
                                {"Type": "Spacer", "Slot": {"Size": {"Rule": "Fill", "Value": 1}}},
                                {"Type": "ComboBoxString", "Name": "ProfileDropdown"},
                                {"Type": "EditableTextBox", "Name": "ProfileNameInput"},
                                {"Type": "Button", "Name": "Btn_New"},
                                {"Type": "Button", "Name": "Btn_Load"},
                                {"Type": "Button", "Name": "Btn_Save"},
                                {"Type": "Button", "Name": "Btn_Validate"},
                                {"Type": "Button", "Name": "Btn_Format"},
                                {"Type": "Button", "Name": "Btn_TestSpawn"}
                            ]
                        },
                        {
                            "Type": "HorizontalBox",
                            "Name": "ContentArea",
                            "Slot": {"Size": {"Rule": "Fill", "Value": 1}},
                            "Children": [
                                {
                                    "Type": "VerticalBox",
                                    "Name": "LeftPanel",
                                    "Slot": {"Size": {"Rule": "Fill", "Value": 0.3}},
                                    "Children": [
                                        {"Type": "TextBlock", "Name": "StatesPanelLabel", "Text": "States"},
                                        {"Type": "ScrollBox", "Name": "StatesPanel", "Slot": {"Size": {"Rule": "Fill", "Value": 1}}}
                                    ]
                                },
                                {
                                    "Type": "VerticalBox",
                                    "Name": "CenterPanel",
                                    "Slot": {"Size": {"Rule": "Fill", "Value": 0.4}},
                                    "Children": [
                                        {"Type": "TextBlock", "Name": "JsonEditorLabel", "Text": "JSON Editor"},
                                        {"Type": "MultiLineEditableTextBox", "Name": "JsonEditor", "Slot": {"Size": {"Rule": "Fill", "Value": 1}}},
                                        {"Type": "TextBlock", "Name": "ValidationText", "Text": ""}
                                    ]
                                },
                                {
                                    "Type": "VerticalBox",
                                    "Name": "RightPanel",
                                    "Slot": {"Size": {"Rule": "Fill", "Value": 0.3}},
                                    "Children": [
                                        {"Type": "TextBlock", "Name": "InspectorPanelLabel", "Text": "Inspector"},
                                        {"Type": "ScrollBox", "Name": "InspectorPanel", "Slot": {"Size": {"Rule": "Fill", "Value": 1}}}
                                    ]
                                }
                            ]
                        },
                        {
                            "Type": "HorizontalBox",
                            "Name": "StatusContainer",
                            "Slot": {"Size": {"Rule": "Auto"}, "Padding": {"Top": 5}},
                            "Children": [
                                {"Type": "TextBlock", "Name": "StatusText", "Text": "Ready", "FontSize": 12}
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
            {"Name": "ProfileNameInput", "Type": "UEditableTextBox", "Purpose": "Profile name input"},
            {"Name": "JsonEditor", "Type": "UMultiLineEditableTextBox", "Purpose": "JSON Editor text area"},
            {"Name": "StatusText", "Type": "UTextBlock", "Purpose": "Status text display"},
            {"Name": "ValidationText", "Type": "UTextBlock", "Purpose": "Validation result text"},
            {"Name": "StatesPanel", "Type": "UScrollBox", "Purpose": "States list container"},
            {"Name": "InspectorPanel", "Type": "UScrollBox", "Purpose": "Inspector panel container"},
            {"Name": "Btn_New", "Type": "UButton", "Purpose": "Create new profile"},
            {"Name": "Btn_Load", "Type": "UButton", "Purpose": "Load profile"},
            {"Name": "Btn_Save", "Type": "UButton", "Purpose": "Save profile"},
            {"Name": "Btn_Validate", "Type": "UButton", "Purpose": "Validate JSON"},
            {"Name": "Btn_Format", "Type": "UButton", "Purpose": "Format JSON"},
            {"Name": "Btn_TestSpawn", "Type": "UButton", "Purpose": "Test spawn AI"}
        ],
        "Optional": [
            {"Name": "TitleText", "Type": "UTextBlock", "Purpose": "Title label"}
        ]
    },
    
    "Dependencies": [
        "/Engine/EngineFonts/Roboto.Roboto"
    ],
    
    "Comments": {
        "Header": "EAIS AI Editor - Visual editor for JSON-based AI behaviors",
        "Usage": "Open via Tools > EAIS > AI Editor menu"
    }
})JSON";
    return Spec;
}
