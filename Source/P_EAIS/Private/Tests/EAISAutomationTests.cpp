/*
 * @Author: Punal Manalan
 * @Description: Implementation of EAIS Automation Tests
 * @Date: 29/12/2025
 */

#include "EAISAutomationTests.h"
#include "PEAIS.h"
#include "AIBehaviour.h"
#include "AIInterpreter.h"
#include "AIAction.h"
#include "EAIS_Types.h"
#include "EAIS_ProfileUtils.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// ==============================================================================
// EAIS.Core.JsonParsing
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISJsonParsingTest, "EAIS.Core.JsonParsing",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISJsonParsingTest::RunTest(const FString &Parameters)
{
    // Test JSON parsing with canonical array-based format
    FString TestJson = TEXT(R"({
        "name": "TestBehavior",
        "initialState": "StateA",
        "blackboard": [
            { "key": "TestBool", "value": { "type": "Bool", "rawValue": "true" } },
            { "key": "TestFloat", "value": { "type": "Float", "rawValue": "0.5" } }
        ],
        "states": [
            {
                "id": "StateA",
                "terminal": false,
                "onEnter": [{ "actionName": "Wait", "paramsJson": "{}" }],
                "onTick": [],
                "onExit": [],
                "transitions": [
                    { "to": "StateB", "priority": 100, "condition": { "type": "Timer", "keyOrName": "", "op": "GreaterThan", "compareValue": { "type": "Float", "rawValue": "0" }, "seconds": 1.0 } }
                ]
            },
            {
                "id": "StateB",
                "terminal": true,
                "onEnter": [],
                "onTick": [],
                "onExit": [],
                "transitions": []
            }
        ]
    })");

    UAIBehaviour *Behavior = NewObject<UAIBehaviour>();
    Behavior->EmbeddedJson = TestJson;

    FString Error;
    bool bParsed = Behavior->ParseBehavior(Error);

    TestTrue(TEXT("JSON should parse successfully"), bParsed);
    TestTrue(TEXT("Parse error should be empty"), Error.IsEmpty());
    TestTrue(TEXT("Behavior should be valid"), Behavior->IsValid());

    const FAIBehaviorDef &Def = Behavior->GetBehaviorDef();
    TestEqual(TEXT("Behavior name should match"), Def.Name, FString(TEXT("TestBehavior")));
    TestEqual(TEXT("Should have 2 states"), Def.States.Num(), 2);
    TestEqual(TEXT("Should have 2 blackboard entries"), Def.Blackboard.Num(), 2);

    return true;
}

// ==============================================================================
// EAIS.Core.BlackboardValues
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISBlackboardTest, "EAIS.Core.BlackboardValues",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISBlackboardTest::RunTest(const FString &Parameters)
{
    // Test FBlackboardValue
    FBlackboardValue BoolVal(true);
    TestEqual(TEXT("Bool type"), BoolVal.Type, EBlackboardValueType::Bool);
    TestEqual(TEXT("Bool value"), BoolVal.BoolValue, true);
    TestEqual(TEXT("Bool ToString"), BoolVal.ToString(), FString(TEXT("true")));

    FBlackboardValue FloatVal(3.14f);
    TestEqual(TEXT("Float type"), FloatVal.Type, EBlackboardValueType::Float);
    TestTrue(TEXT("Float value"), FMath::IsNearlyEqual(FloatVal.FloatValue, 3.14f));

    FBlackboardValue VecVal(FVector(1, 2, 3));
    TestEqual(TEXT("Vector type"), VecVal.Type, EBlackboardValueType::Vector);
    TestEqual(TEXT("Vector value"), VecVal.VectorValue, FVector(1, 2, 3));

    // Test comparison
    FBlackboardValue A(10.0f);
    FBlackboardValue B(5.0f);
    TestTrue(TEXT("Greater than"), A.Compare(B, EAIConditionOperator::GreaterThan));
    TestFalse(TEXT("Less than"), A.Compare(B, EAIConditionOperator::LessThan));
    TestTrue(TEXT("Not equal"), A.Compare(B, EAIConditionOperator::NotEqual));

    return true;
}

// ==============================================================================
// EAIS.ActionsRegistry.RegisterAndInvoke (Section 10.1)
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISActionsRegistryTest, "EAIS.ActionsRegistry.RegisterAndInvoke",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISActionsRegistryTest::RunTest(const FString &Parameters)
{
    // Test that built-in actions are registered
    // Note: Full registry testing requires a World context for subsystem access

    // Verify action class types exist
    TestNotNull(TEXT("MoveTo action class exists"), UAIAction_MoveTo::StaticClass());
    TestNotNull(TEXT("Kick action class exists"), UAIAction_Kick::StaticClass());
    TestNotNull(TEXT("Wait action class exists"), UAIAction_Wait::StaticClass());
    TestNotNull(TEXT("InjectInput action class exists"), UAIAction_InjectInput::StaticClass());

    // Verify action names
    UAIAction_MoveTo *MoveToAction = NewObject<UAIAction_MoveTo>();
    TestEqual(TEXT("MoveTo action name"), MoveToAction->GetActionName(), FString(TEXT("MoveTo")));

    UAIAction_Kick *KickAction = NewObject<UAIAction_Kick>();
    TestEqual(TEXT("Kick action name"), KickAction->GetActionName(), FString(TEXT("Kick")));

    return true;
}

// ==============================================================================
// EAIS.Core.InterpreterInit
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISInterpreterInitTest, "EAIS.Core.InterpreterInit",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISInterpreterInitTest::RunTest(const FString &Parameters)
{
    FString TestJson = TEXT(R"({
        "name": "InitTest",
        "initialState": "Initial",
        "blackboard": [{ "key": "Counter", "value": { "type": "Int", "rawValue": "0" } }],
        "states": [
            { "id": "Initial", "terminal": true, "onEnter": [], "onTick": [], "onExit": [], "transitions": [] }
        ]
    })");

    FAIInterpreter Interpreter;
    FString Error;
    bool bLoaded = Interpreter.LoadFromJson(TestJson, Error);

    TestTrue(TEXT("Should load JSON"), bLoaded);
    TestEqual(TEXT("Behavior name"), Interpreter.GetBehaviorName(), FString(TEXT("InitTest")));

    // After Initialize/Reset, should be in initial state
    TArray<FString> States = Interpreter.GetAllStateIds();
    TestEqual(TEXT("Should have 1 state"), States.Num(), 1);
    TestTrue(TEXT("Should contain Initial state"), States.Contains(TEXT("Initial")));

    return true;
}

// ==============================================================================
// EAIS.Core.StateTransition
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISStateTransitionTest, "EAIS.Core.StateTransition",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISStateTransitionTest::RunTest(const FString &Parameters)
{
    // This test validates the behavior definition parsing for transitions
    FString TestJson = TEXT(R"({
        "name": "TransitionTest",
        "initialState": "A",
        "states": [
            {
                "id": "A",
                "terminal": false,
                "onEnter": [], "onTick": [], "onExit": [],
                "transitions": [
                    { "to": "B", "priority": 100, "condition": { "type": "Timer", "keyOrName": "", "op": "GreaterThan", "compareValue": { "type": "Float", "rawValue": "0" }, "seconds": 0.5 } }
                ]
            },
            {
                "id": "B",
                "terminal": false,
                "onEnter": [], "onTick": [], "onExit": [],
                "transitions": [
                    { "to": "C", "priority": 100, "condition": { "type": "Event", "keyOrName": "TestEvent", "op": "Equal", "compareValue": { "type": "Bool", "rawValue": "true" } } }
                ]
            },
            { "id": "C", "terminal": true, "onEnter": [], "onTick": [], "onExit": [], "transitions": [] }
        ]
    })");

    FAIInterpreter Interpreter;
    FString Error;
    TestTrue(TEXT("Should parse"), Interpreter.LoadFromJson(TestJson, Error));

    TArray<FString> States = Interpreter.GetAllStateIds();
    TestEqual(TEXT("Should have 3 states"), States.Num(), 3);

    return true;
}

// ==============================================================================
// EAIS.Core.EventHandling
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISEventHandlingTest, "EAIS.Core.EventHandling",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISEventHandlingTest::RunTest(const FString &Parameters)
{
    FString TestJson = TEXT(R"({
        "name": "EventTest",
        "initialState": "Waiting",
        "blackboard": [{ "key": "EventReceived", "value": { "type": "Bool", "rawValue": "false" } }],
        "states": [
            {
                "id": "Waiting",
                "terminal": false,
                "onEnter": [], "onTick": [], "onExit": [],
                "transitions": [
                    { "to": "Done", "priority": 100, "condition": { "type": "Event", "keyOrName": "MyEvent", "op": "Equal", "compareValue": { "type": "Bool", "rawValue": "true" } } }
                ]
            },
            { "id": "Done", "terminal": true, "onEnter": [], "onTick": [], "onExit": [], "transitions": [] }
        ]
    })");

    FAIInterpreter Interpreter;
    FString Error;
    TestTrue(TEXT("Should parse"), Interpreter.LoadFromJson(TestJson, Error));

    // Enqueue an event
    FAIEventPayload Payload;
    Interpreter.EnqueueEvent(TEXT("MyEvent"), Payload);

    // The event should be queued (full transition requires component/tick)
    return true;
}

// ==============================================================================
// EAIS.Integration.JsonSchema
// ==============================================================================

// ==============================================================================
// EAIS.Core.ProfileUtils
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISProfileUtilsTest, "EAIS.Core.ProfileUtils",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISProfileUtilsTest::RunTest(const FString &Parameters)
{
    TSet<FString> Names;
    Names.Add(TEXT("Goalkeeper"));
    Names.Add(TEXT("Striker"));
    Names.Add(TEXT("Defender"));

    const TArray<FString> Sorted = EAIS_ProfileUtils::MakeSortedUnique(Names);
    TestEqual(TEXT("Sorted list should contain 3 items"), Sorted.Num(), 3);
    TestEqual(TEXT("Sorted[0] should be Defender"), Sorted[0], FString(TEXT("Defender")));
    TestEqual(TEXT("Sorted[1] should be Goalkeeper"), Sorted[1], FString(TEXT("Goalkeeper")));
    TestEqual(TEXT("Sorted[2] should be Striker"), Sorted[2], FString(TEXT("Striker")));

    const FString PreferredExact = EAIS_ProfileUtils::ChooseDefaultProfile(Sorted, TEXT("Striker"));
    TestEqual(TEXT("Preferred exact match should return Striker"), PreferredExact, FString(TEXT("Striker")));

    const FString PreferredCaseInsensitive = EAIS_ProfileUtils::ChooseDefaultProfile(Sorted, TEXT("sTriKer"));
    TestEqual(TEXT("Preferred case-insensitive match should return Striker"), PreferredCaseInsensitive, FString(TEXT("Striker")));

    const FString MissingPreferred = EAIS_ProfileUtils::ChooseDefaultProfile(Sorted, TEXT("Missing"));
    TestEqual(TEXT("Missing preferred should fall back to first"), MissingPreferred, FString(TEXT("Defender")));

    const FString EmptyPreferred = EAIS_ProfileUtils::ChooseDefaultProfile(Sorted, TEXT(""));
    TestEqual(TEXT("Empty preferred should fall back to first"), EmptyPreferred, FString(TEXT("Defender")));

    const TArray<FString> Empty;
    const FString EmptyResult = EAIS_ProfileUtils::ChooseDefaultProfile(Empty, TEXT("Striker"));
    TestTrue(TEXT("Empty list should return empty"), EmptyResult.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISJsonSchemaTest, "EAIS.Integration.JsonSchema",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISJsonSchemaTest::RunTest(const FString &Parameters)
{
    // Test that the example JSONs parse correctly
    TArray<FString> ProfileNames = {TEXT("Striker"), TEXT("Goalkeeper"), TEXT("Defender")};

    for (const FString &Name : ProfileNames)
    {
        FString FilePath = FPaths::ProjectContentDir() / TEXT("AIProfiles") / (Name + TEXT(".json"));

        if (FPaths::FileExists(FilePath))
        {
            FString JsonContent;
            FFileHelper::LoadFileToString(JsonContent, *FilePath);

            UAIBehaviour *Behavior = NewObject<UAIBehaviour>();
            Behavior->EmbeddedJson = JsonContent;

            FString Error;
            bool bParsed = Behavior->ParseBehavior(Error);

            TestTrue(FString::Printf(TEXT("%s should parse"), *Name), bParsed);
            TestTrue(FString::Printf(TEXT("%s should be valid"), *Name), Behavior->IsValid());

            if (!bParsed)
            {
                AddError(FString::Printf(TEXT("%s parse error: %s"), *Name, *Error));
            }
        }
        else
        {
            AddWarning(FString::Printf(TEXT("Profile not found: %s"), *FilePath));
        }
    }

    return true;
}

// ==============================================================================
// EAIS.Functional.TickExecution
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISTickExecutionTest, "EAIS.Functional.TickExecution",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISTickExecutionTest::RunTest(const FString &Parameters)
{
    // This is a placeholder for functional tests that require a world
    // Full functional tests should be implemented as FunctionalTests in a test map

    // For now, just verify the interpreter can be created
    FAIInterpreter Interpreter;

    FString TestJson = TEXT(R"({
        "name": "TickTest",
        "initialState": "Idle",
        "states": [{ "id": "Idle", "terminal": true, "onEnter": [], "onTick": [], "onExit": [], "transitions": [] }]
    })");

    FString Error;
    Interpreter.LoadFromJson(TestJson, Error);

    // Reset would normally require a component, but definitions should be loaded
    TArray<FString> States = Interpreter.GetAllStateIds();
    TestEqual(TEXT("Should have loaded state"), States.Num(), 1);

    return true;
}

// ==============================================================================
// EAIS.Core.CompositeConditions
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISCompositeConditionTest, "EAIS.Core.CompositeConditions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISCompositeConditionTest::RunTest(const FString &Parameters)
{
    FAIInterpreter Interpreter;
    Interpreter.SetBlackboardBool(TEXT("Key1"), true);
    Interpreter.SetBlackboardBool(TEXT("Key2"), true);

    // Test And condition (True AND True = True)
    FAICondition AndCond;
    AndCond.Type = EAIConditionType::And;
    
    FAICondition Sub1;
    Sub1.Type = EAIConditionType::Blackboard;
    Sub1.Name = TEXT("Key1");
    Sub1.Value = TEXT("true");
    
    FAICondition Sub2;
    Sub2.Type = EAIConditionType::Blackboard;
    Sub2.Name = TEXT("Key2");
    Sub2.Value = TEXT("true");
    
    AndCond.SubConditions.Add(Sub1);
    AndCond.SubConditions.Add(Sub2);
    
    TestTrue(TEXT("TRUE AND TRUE should be TRUE"), Interpreter.EvaluateCondition(AndCond));
    
    Interpreter.SetBlackboardBool(TEXT("Key2"), false);
    TestFalse(TEXT("TRUE AND FALSE should be FALSE"), Interpreter.EvaluateCondition(AndCond));

    // Test Or condition (True OR False = True)
    FAICondition OrCond;
    OrCond.Type = EAIConditionType::Or;
    OrCond.SubConditions.Add(Sub1);
    OrCond.SubConditions.Add(Sub2);
    
    TestTrue(TEXT("TRUE OR FALSE should be TRUE"), Interpreter.EvaluateCondition(OrCond));
    
    Interpreter.SetBlackboardBool(TEXT("Key1"), false);
    TestFalse(TEXT("FALSE OR FALSE should be FALSE"), Interpreter.EvaluateCondition(OrCond));

    // Test Not condition (NOT False = True)
    FAICondition NotCond;
    NotCond.Type = EAIConditionType::Not;
    NotCond.SubConditions.Add(Sub1); // Key1 is now false
    
    TestTrue(TEXT("NOT FALSE should be TRUE"), Interpreter.EvaluateCondition(NotCond));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
