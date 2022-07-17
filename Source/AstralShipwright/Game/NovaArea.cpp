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

TArray<const UNovaResource*> UNovaArea::GetResourcesBought() const
{
	TArray<const UNovaResource*> Result;

	for (const UNovaResource* Resource : UNeutronAssetManager::Get()->GetAssets<UNovaResource>())
	{
		if (!IsResourceSold(Resource))
		{
			Result.Add(Resource);
		}
	}

	return Result;
}

TArray<const UNovaResource*> UNovaArea::GetResourcesSold() const
{
	TArray<const UNovaResource*> Result;

	for (const FNovaResourceTrade& Trade : ResourceTradeMetadata)
	{
		if (Trade.ForSale)
		{
			Result.Add(Trade.Resource);
		}
	}

	return Result;
}
