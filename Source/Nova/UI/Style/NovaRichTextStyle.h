// Nova project - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "NovaRichTextStyle.generated.h"

/*----------------------------------------------------
    Wrapper classes
----------------------------------------------------*/

UCLASS()
class UNovaRichTextStyleContainer : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:
	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast<const struct FSlateWidgetStyle*>(&Style);
	}

	UPROPERTY(Category = Nova, EditDefaultsOnly, meta = (ShowOnlyInnerProperties))
	FInlineTextImageStyle Style;
};
