// Astral Shipwright - Gwennaël Arbona

#include "NovaArea.h"
#include "Nova.h"

bool UNovaArea::IsResourceSold(const UNovaResource* Asset) const
{
	for (const FNovaResourceTrade& Trade : ResourceTradeMetadata)
	{
		if (Trade.Resource == Asset)
		{
			return Trade.ForSale;
		}
	}

	return false;
}

TArray<const UNovaResource*> UNovaArea::GetResourcesBought(ENovaResourceType Type) const
{
	TArray<const UNovaResource*> Result;

	for (const UNovaResource* Resource : UNeutronAssetManager::Get()->GetAssets<UNovaResource>())
	{
		if (!IsResourceSold(Resource) && Resource->Type == Type)
		{
			Result.Add(Resource);
		}
	}

	return Result;
}

TArray<const UNovaResource*> UNovaArea::GetResourcesSold(ENovaResourceType Type) const
{
	TArray<const UNovaResource*> Result;

	for (const FNovaResourceTrade& Trade : ResourceTradeMetadata)
	{
		if (Trade.ForSale && Trade.Resource->Type == Type)
		{
			Result.Add(Trade.Resource);
		}
	}

	return Result;
}
