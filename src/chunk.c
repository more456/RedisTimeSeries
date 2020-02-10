/*
* Copyright 2018-2019 Redis Labs Ltd. and Contributors
*
* This file is available under the Redis Labs Source Available License Agreement
*/
#include "chunk.h"
#include <string.h>
#include "rmutil/alloc.h"

Chunk_t *Uncompressed_NewChunk(size_t sampleCount) {
    Chunk *newChunk = (Chunk *)malloc(sizeof(Chunk));
    newChunk->num_samples = 0;
    newChunk->max_samples = sampleCount;
    newChunk->samples = (Sample *)malloc(sizeof(Sample) * sampleCount);

    return newChunk;
}

void Uncompressed_FreeChunk(Chunk_t *chunk) {
    free(((Chunk *)chunk)->samples);
    free(chunk);
}

static int IsChunkFull(Chunk *chunk) {
    return chunk->num_samples == chunk->max_samples;
}

u_int64_t Uncompressed_NumOfSample(Chunk_t *chunk) {
    return ((Chunk *)chunk)->num_samples;
}

static Sample *ChunkGetSample(Chunk *chunk, int index) {
    return &chunk->samples[index];
}

timestamp_t Uncompressed_GetLastTimestamp(Chunk_t *chunk) {
    if (((Chunk *)chunk)->num_samples == 0) {
        return -1;
    }
    return ChunkGetSample(chunk, ((Chunk *)chunk)->num_samples - 1)->timestamp;
}

timestamp_t Uncompressed_GetFirstTimestamp(Chunk_t *chunk) {
    if (((Chunk *)chunk)->num_samples == 0) {
        return -1;
    }
    return ChunkGetSample(chunk, 0)->timestamp;
}

ChunkResult Uncompressed_AddSample(Chunk_t *chunk, Sample *sample) {
    Chunk *regChunk = (Chunk *)chunk;
    if (IsChunkFull(regChunk)){
        return CR_END;
    }

    if (Uncompressed_NumOfSample(regChunk) == 0) {
        // initialize base_timestamp
        regChunk->base_timestamp = sample->timestamp;
    }

    regChunk->samples[regChunk->num_samples] = *sample;
    regChunk->num_samples++;

    return CR_OK;
}

ChunkIter_t *Uncompressed_NewChunkIterator(Chunk_t *chunk, bool rev) {
    ChunkIterator *iter = (ChunkIterator *)calloc(1, sizeof(ChunkIterator));
    iter->chunk = chunk;
    iter->currentIndex = (rev == false) ? 0 : iter->chunk->num_samples - 1;
    return (ChunkIter_t *)iter;
}

ChunkResult Uncompressed_ChunkIteratorGetNext(ChunkIter_t *iterator, Sample *sample, bool rev) {
    ChunkIterator *iter = (ChunkIterator *)iterator;
    if (rev == false) {
        if (iter->currentIndex < iter->chunk->num_samples) {
            *sample = *ChunkGetSample(iter->chunk, iter->currentIndex);
            iter->currentIndex++;
            return CR_OK;
        }
    } else if (iter->currentIndex >= 0) {
        *sample = *ChunkGetSample(iter->chunk, iter->currentIndex);
        iter->currentIndex--;
        return CR_OK;
    } 
    // no more samples in the chunk
    return CR_END;
}

void Uncompressed_FreeChunkIterator(ChunkIter_t *iter, bool rev) {
    (void)rev; // only used with compressed chunk but signature must be similar
    free(iter);
}

size_t Uncompressed_GetChunkSize(Chunk_t *chunk) {
    return sizeof(Chunk) + ((Chunk *)chunk)->max_samples * sizeof(Sample);
}