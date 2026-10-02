#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"

/** Attribute Set의 속성마다 GAS 표준 접근자(Attribute, 값 읽기·쓰기·초기화)를 만든다. */
#define PD_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
