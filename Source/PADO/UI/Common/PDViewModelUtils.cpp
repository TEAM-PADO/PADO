// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Common/PDViewModelUtils.h"

#include "Blueprint/UserWidget.h"
#include "MVVMSubsystem.h"
#include "MVVMViewModelBase.h"
#include "PADO/UI/PDUILog.h"
#include "View/MVVMView.h"

bool PDViewModelUtils::SetViewModel(UUserWidget* Widget, UMVVMViewModelBase* ViewModel)
{
	if (!Widget || !ViewModel)
	{
		return false;
	}

	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(Widget);
	if (!View)
	{
		UE_LOG(LogPDUI, Warning, TEXT("%s has no MVVM view. Add %s to its Viewmodels panel with Creation Type 'Manual'."),
			*GetNameSafe(Widget->GetClass()), *GetNameSafe(ViewModel->GetClass()));
		return false;
	}

	if (!View->SetViewModelByClass(ViewModel))
	{
		UE_LOG(LogPDUI, Warning, TEXT("%s could not accept viewmodel %s. Check the Viewmodels panel class and Creation Type 'Manual'."),
			*GetNameSafe(Widget->GetClass()), *GetNameSafe(ViewModel->GetClass()));
		return false;
	}

	return true;
}
