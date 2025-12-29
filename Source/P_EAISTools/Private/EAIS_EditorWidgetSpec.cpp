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
    
    return TEXT(R"({
  "WidgetClass": "EUW_EAIS_Editor",
  "WidgetType": "EditorUtilityWidget",
  "RootWidget": {
    "Type": "VerticalBox",
    "Padding": { "Left": 10, "Top": 10, "Right": 10, "Bottom": 10 },
    "Children": [
      {
        "Type": "HorizontalBox",
        "Slot": { "AutoSize": true },
        "Children": [
          {
            "Type": "TextBlock",
            "Text": "EAIS Editor",
            "Font": { "Size": 24, "TypeFace": "Bold" }
          },
          {
            "Type": "Spacer",
            "Slot": { "FillWidth": 1.0 }
          },
          {
            "Type": "Button",
            "Name": "Btn_Load",
            "Text": "Load",
            "OnClicked": "OnLoadClicked"
          },
          {
            "Type": "Button",
            "Name": "Btn_Save",
            "Text": "Save",
            "OnClicked": "OnSaveClicked"
          },
          {
            "Type": "Button",
            "Name": "Btn_Validate",
            "Text": "Validate",
            "OnClicked": "OnValidateClicked"
          }
        ]
      },
      {
        "Type": "Separator"
      },
      {
        "Type": "HorizontalBox",
        "Slot": { "FillHeight": 1.0 },
        "Children": [
          {
            "Type": "VerticalBox",
            "Slot": { "FillWidth": 0.25 },
            "Children": [
              {
                "Type": "TextBlock",
                "Text": "Behaviors",
                "Font": { "Size": 14, "TypeFace": "Bold" }
              },
              {
                "Type": "ListView",
                "Name": "BehaviorList",
                "Slot": { "FillHeight": 1.0 }
              }
            ]
          },
          {
            "Type": "Separator",
            "Orientation": "Vertical"
          },
          {
            "Type": "VerticalBox",
            "Slot": { "FillWidth": 0.5 },
            "Children": [
              {
                "Type": "TextBlock",
                "Text": "State Graph",
                "Font": { "Size": 14, "TypeFace": "Bold" }
              },
              {
                "Type": "Border",
                "Name": "GraphContainer",
                "Slot": { "FillHeight": 1.0 },
                "Background": { "Color": "#1a1a1a" }
              }
            ]
          },
          {
            "Type": "Separator",
            "Orientation": "Vertical"
          },
          {
            "Type": "VerticalBox",
            "Slot": { "FillWidth": 0.25 },
            "Children": [
              {
                "Type": "TextBlock",
                "Text": "Inspector",
                "Font": { "Size": 14, "TypeFace": "Bold" }
              },
              {
                "Type": "ScrollBox",
                "Name": "InspectorPanel",
                "Slot": { "FillHeight": 1.0 }
              }
            ]
          }
        ]
      },
      {
        "Type": "Separator"
      },
      {
        "Type": "HorizontalBox",
        "Slot": { "AutoSize": true },
        "Children": [
          {
            "Type": "TextBlock",
            "Name": "StatusText",
            "Text": "Ready"
          },
          {
            "Type": "Spacer",
            "Slot": { "FillWidth": 1.0 }
          },
          {
            "Type": "Button",
            "Name": "Btn_Preview",
            "Text": "Run Preview",
            "OnClicked": "OnPreviewClicked"
          },
          {
            "Type": "Button",
            "Name": "Btn_Stop",
            "Text": "Stop",
            "OnClicked": "OnStopClicked"
          }
        ]
      }
    ]
  },
  "Events": {
    "OnLoadClicked": "LoadBehaviorFromFile",
    "OnSaveClicked": "SaveBehaviorToFile",
    "OnValidateClicked": "ValidateBehavior",
    "OnPreviewClicked": "StartPreview",
    "OnStopClicked": "StopPreview"
  }
})");
}
