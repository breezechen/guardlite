/*
 *	�ؼ���غ���
 */
#include "EhomeNet.h"
#include "Keyword.h"

#define MAX_INDEX					256

typedef struct 
{
	LIST_ENTRY		list;
	ULONG			nSize;
	int				type;
} keyword_item, *keyword_item_ptr;

typedef struct 
{
	BOOLEAN			iskey;
	LIST_ENTRY		list;
} keyword_index, *keyword_index_ptr;

KSPIN_LOCK			g_keyword_SpinLock		= {0};
keyword_index_ptr	g_keyword_Index			= NULL;

/* ��ʼ���ؼ��� */
void keyword_Init()
{
	int			i		= 0;

	KeInitializeSpinLock(&g_keyword_SpinLock);
	g_keyword_Index = (keyword_index_ptr)ExAllocatePoolWithTag(NonPagedPool, sizeof(keyword_index) * MAX_INDEX, 'ehom');
	ASSERT(NULL != g_keyword_Index);
	if(NULL == g_keyword_Index)
		return;
	for(i = 0; i < MAX_INDEX; i++)
	{
		InitializeListHead(&g_keyword_Index[i].list);
		g_keyword_Index[i].iskey = FALSE;
	}
}

/* ���ٹؼ��� */
void keyword_Release()
{
	int					i				= 0;
	PLIST_ENTRY			pList			= NULL;
	keyword_item_ptr	pKeywordItem	= NULL;
	KIRQL				oldIrql;

	KeAcquireSpinLock(&g_keyword_SpinLock, &oldIrql);
	if(NULL != g_keyword_Index)
	{
		for(i = 0; i < MAX_INDEX; i++)
		{
			for(pList = g_keyword_Index[i].list.Blink
				; pList != &g_keyword_Index[i].list
			; pList = pList->Blink)
			{
				pKeywordItem = CONTAINING_RECORD(pList, keyword_item, list);
				pList->Blink->Flink = pList->Flink;
				pList->Flink->Blink = pList->Blink;
				ExFreePoolWithTag(pKeywordItem, 'ehom');
			}
		}
		ExFreePoolWithTag(g_keyword_Index, 'ehom');
		g_keyword_Index = NULL;
	}
	KeReleaseSpinLock(&g_keyword_SpinLock, oldIrql);
}

/* ���Keyword */
void keyword_Clear()
{
	int					i				= 0;
	PLIST_ENTRY			pList			= NULL;
	keyword_item_ptr	pKeywordItem	= NULL;
	KIRQL				oldIrql;

	KeAcquireSpinLock(&g_keyword_SpinLock, &oldIrql);
	if(NULL != g_keyword_Index)
	{
		for(i = 0; i < MAX_INDEX; i++)
		{
			for(pList = g_keyword_Index[i].list.Blink
				; pList != &g_keyword_Index[i].list
				; pList = pList->Blink)
			{
				pKeywordItem = CONTAINING_RECORD(pList, keyword_item, list);
				pList->Blink->Flink = pList->Flink;
				pList->Flink->Blink = pList->Blink;
				ExFreePoolWithTag(pKeywordItem, 'ehom');
			}
			g_keyword_Index[i].iskey = FALSE;
		}
	}
	KeReleaseSpinLock(&g_keyword_SpinLock, oldIrql);
}

/* ���һ��KEY */
void keyword_Add(int type, char* pKeyword, ULONG nLen)
{
	keyword_item_ptr	pKeyItem	= NULL;
	PLIST_ENTRY			pList		= NULL;
	int					i;
	KIRQL				oldIrql;

	if(NULL == pKeyword || 0 == nLen)
		return;
	// ȥ��0�ؼ���
	for(i = (int)nLen-1; i >= 0; i--)
	{
		if('\x0' == pKeyword[i])
			nLen--;
		else
			break;
	}
	if(0 == nLen)
		return;
	// ��ʼ���
	KeAcquireSpinLock(&g_keyword_SpinLock, &oldIrql);
	if(NULL != g_keyword_Index)
	{
		UCHAR			uchar		= (UCHAR)pKeyword[0];
		BOOL			bAdd		= TRUE;

		if(uchar >= 'A' && uchar <= 'Z')
			uchar = uchar - 'A' + 'a';
		for(pList = g_keyword_Index[uchar].list.Blink
			; pList != &g_keyword_Index[uchar].list
			; pList = pList->Blink)
		{
			pKeyItem = CONTAINING_RECORD(pList, keyword_item, list);
			if(nLen != pKeyItem->nSize)
				continue;
			if(memcmp((char *)pKeyItem + sizeof(keyword_item), pKeyword, nLen) == 0)
			{
				bAdd = FALSE;
				break;
			}
		}
		if(bAdd)
		{
			pKeyItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(keyword_item) + nLen, 'ehom');
			if(NULL != pKeyItem)
			{
				ULONG		k;
				char*		pAddKey		= (char*)pKeyItem + sizeof(keyword_item);

				// ��ӹؼ���
				pKeyItem->type = type;
				pKeyItem->nSize = nLen;
				memcpy(pAddKey, pKeyword, nLen);
				InsertHeadList(&g_keyword_Index[uchar].list, &pKeyItem->list);
				g_keyword_Index[uchar].iskey = TRUE;
				// ���ؼ���ת��ΪСд
				for(k = 0; k < pKeyItem->nSize; k++)
				{
					if(pAddKey[k] >='A' && pAddKey[k] <= 'Z')
					{
						pAddKey[k] = pAddKey[k] - 'A' + 'a';
					}
				}
			}
		}
	}
	KeReleaseSpinLock(&g_keyword_SpinLock, oldIrql);
}

/* ���ҹؼ��� */
BOOLEAN keyword_Find(IN char* pData, IN int nLenData, OUT char** ppKeyWord, 
	OUT int* pLenKeyWord, OUT int* pType)
{
	int				i			= 0;
	BOOLEAN			bFind		= FALSE;
 	KIRQL			oldIrql;

	KeAcquireSpinLock(&g_keyword_SpinLock, &oldIrql);
	__try
	{
		// ��ʼ���Ҳ���
		for(i = 0; i < nLenData && FALSE == bFind; i++)
		{
			UCHAR					uchar			= (UCHAR)pData[i];
			PLIST_ENTRY				pList;
			keyword_item_ptr		pKeyItem;
			CHAR					szKey[129];

			if(uchar >= 'A' && uchar <= 'Z')
				uchar = uchar - 'A' + 'a';
			if(FALSE == g_keyword_Index[uchar].iskey)
				continue;	// ��������
			// ������Ҫ�ȽϵĻ�����
			memset(szKey, 0, sizeof(szKey));
			memcpy(szKey, pData + i, min(128, nLenData - i));
			_strlwr(szKey);
			// ��ʼ���ַ������Ҳ�
			for(pList = g_keyword_Index[uchar].list.Blink
				; pList != &g_keyword_Index[uchar].list
				; pList = pList->Blink)
			{
				pKeyItem = CONTAINING_RECORD(pList, keyword_item, list);
				if( (i + (int)pKeyItem->nSize) > nLenData )
					continue;
				if(memcmp(szKey, (char*)pKeyItem+sizeof(keyword_item), pKeyItem->nSize) == 0)
				{
					bFind = TRUE;
					*ppKeyWord = pData + i;
					*pLenKeyWord = pKeyItem->nSize;
					*pType = pKeyItem->type;
					break;
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// ��ֹ��ȡԽ���쳣
		bFind = FALSE;
	}

	KeReleaseSpinLock(&g_keyword_SpinLock, oldIrql);
	return bFind;
}