#include "map.h"
#include <engine/storage.h>
#include <engine/kernel.h>


bool CMap::Load(const char *pMapName, IKernel *pKernel, IStorage *pStorage)
{
		if(!pKernel)
			return false;

		if(!pStorage)
			return false;


		if(!m_DataFile.Open(pStorage, pMapName, IStorage::TYPE_ALL))
			return false;
		// check version
		CMapItemVersion *pItem = (CMapItemVersion *)m_DataFile.FindItem(MAPITEMTYPE_VERSION, 0);
		if(!pItem)
			return false;

		// replace compressed tile layers with uncompressed ones
		int GroupsStart, GroupsNum, LayersStart, LayersNum;
		m_DataFile.GetType(MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);
		m_DataFile.GetType(MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);
		for(int g = 0; g < GroupsNum; g++)
		{
			CMapItemGroup *pGroup = static_cast<CMapItemGroup *>(m_DataFile.GetItem(GroupsStart + g, 0, 0));
			for(int l = 0; l < pGroup->m_NumLayers; l++)
			{
				CMapItemLayer *pLayer = static_cast<CMapItemLayer *>(m_DataFile.GetItem(LayersStart + pGroup->m_StartLayer + l, 0, 0));

				if(pLayer->m_Type == LAYERTYPE_TILES)
				{
					CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);

					if(pTilemap->m_Version > 3)
					{
						const int TilemapCount = pTilemap->m_Width * pTilemap->m_Height;
						const int TilemapSize = TilemapCount * sizeof(CTile);

						if((TilemapCount / pTilemap->m_Width != pTilemap->m_Height) || (TilemapSize / (int)sizeof(CTile) != TilemapCount))
						{
							dbg_msg("engine", "map layer too big (%d * %d * %u causes an integer overflow)", pTilemap->m_Width, pTilemap->m_Height, unsigned(sizeof(CTile)));
							return false;
						}
						CTile *pTiles = static_cast<CTile *>(mem_alloc(TilemapSize, 1));
						if(!pTiles)
							return false;

						// extract original tile data
						int i = 0;
						CTile *pSavedTiles = static_cast<CTile *>(m_DataFile.GetData(pTilemap->m_Data));
						while(i < TilemapCount)
						{
							for(unsigned Counter = 0; Counter <= pSavedTiles->m_Skip && i < TilemapCount; Counter++)
							{
								pTiles[i] = *pSavedTiles;
								pTiles[i++].m_Skip = 0;
							}

							pSavedTiles++;
						}

						//m_DataFile.ReplaceData(pTilemap->m_Data, reinterpret_cast<char *>(pTiles));
					}
				}
			}

		}
	return true;
}