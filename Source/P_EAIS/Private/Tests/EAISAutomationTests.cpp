/*
 * @Author: Punal Manalan
 * @Description: Implementation of EAIS Automation Tests
 * @Date: 29/12/2025
 */

#include "EAISAutomationTests.h"
#include "PEAIS.h"
#include "AIBehaviour.h"
#include "AIInterpreter.h"
#include "EAIS_Types.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// ==============================================================================
// EAIS.Core.JsonParsing
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISJsonParsingTest, "EAIS.Core.JsonParsing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISJsonParsingTest::RunTest(const FString& Parameters)
{
    // Test JSON parsing
    FString TestJson = TEXT(R"({
        "name": "TestBehavior",
        "blackboard": {
            "TestBool": true,
            "TestFloat": 0.5
        },
        "states": {
            "StateA": {
                "OnEnter": [{ "Action": "Wait" }],
                "Transitions": [
                    { "Target": "StateB", "Condition": { "type": "Timer", "seconds": 1.0 } }
                ]
            },
            "StateB": {
                "Transitions": []
            }
        }
    })");

    UAIBehaviour* Behavior = NewObject<UAIBehaviour>();
    Behavior->EmbeddedJson = TestJson;
    
    FString Error;
    bool bParsed = Behavior->ParseBehavior(Error);
    
    TestTrue(TEXT("JSON should parse successfully"), bParsed);
    TestTrue(TEXT("Parse error should be empty"), Error.IsEmpty());
    TestTrue(TEXT("Behavior should be valid"), Behavior->IsValid());
    
    const FAIBehaviorDef& Def = Behavior->GetBehaviorDef();
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

bool FEAISBlackboardTest::RunTest(const FString& Parameters)
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
// EAIS.Core.InterpreterInit
// ==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISInterpreterInitTest, "EAIS.Core.InterpreterInit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISInterpreterInitTest::RunTest(const FString& Parameters)
{
    FString TestJson = TEXT(R"({
        "name": "InitTest",
        "blackboard": { "Counter": 0 },
        "states": {
            "Initial": { "Transitions": [] }
        }
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

bool FEAISStateTransitionTest::RunTest(const FString& Parameters)
{
    // This test validates the behavior definition parsing for transitions
    FString TestJson = TEXT(R"({
        "name": "TransitionTest",
        "states": {
            "A": {
                "Transitions": [
                    { "Target": "B", "Condition": { "type": "Timer", "seconds": 0.5 } }
                ]
            },
            "B": {
                "Transitions": [
                    { "Target": "C", "Condition": { "type": "Event", "name": "TestEvent" } }
                ]
            },
            "C": { "Transitions": [] }
        }
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

bool FEAISEventHandlingTest::RunTest(const FString& Parameters)
{
    FString TestJson = TEXT(R"({
        "name": "EventTest",
        "blackboard": { "EventReceived": false },
        "states": {
            "Waiting": {
                "Transitions": [
                    { "Target": "Done", "Condition": { "type": "Event", "name": "MyEvent" } }
                ]
            },
            "Done": { "Transitions": [] }
        }
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAISJsonSchemaTest, "EAIS.Integration.JsonSchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEAISJsonSchemaTest::RunTest(const FString& Parameters)
{
    // Test that the example JSONs parse correctly
    TArray<FString> ProfileNames = { TEXT("Striker"), TEXT("Goalkeeper"), TEXT("Defender") };
    
    for (const FString& Name : ProfileNames)
    {
        FString FilePath = FPaths::ProjectContentDir() / TEXT("AIProfiles") / (Name + TEXT(".json"));
        
        if (FPaths::FileExists(FilePath))
        {
            FString JsonContent;
            FFileHelper::LoadFileToString(JsonContent, *FilePath);
            
            UAIBehaviour* Behavior = NewObject<UAIBehaviour>();
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

bool FEAISTickExecutionTest::RunTest(const FString& Parameters)
{
    // This is a placeholder for functional tests that require a world
    // Full functional tests should be implemented as FunctionalTests in a test map
    
    // For now, just verify the interpreter can be created
    FAIInterpreter Interpreter;
    
    FString TestJson = TEXT(R"({
        "name": "TickTest",
        "states": { "Idle": { "Transitions": [] } }
    })");
    
    FString Error;
    Interpreter.LoadFromJson(TestJson, Error);
    
    // Reset would normally require a component, but definitions should be loaded
    TArray<FString> States = Interpreter.GetAllStateIds();
    TestEqual(TEXT("Should have loaded state"), States.Num(), 1);
    
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
