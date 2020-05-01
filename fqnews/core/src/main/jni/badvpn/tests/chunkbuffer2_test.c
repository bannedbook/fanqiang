#include <misc/debug.h>
#include <structure/ChunkBuffer2.h>

int main ()
{
    struct ChunkBuffer2_block blocks[16];
    ChunkBuffer2 buf;
    ChunkBuffer2_Init(&buf, blocks, 16, 4 * sizeof(struct ChunkBuffer2_block));
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.input_avail == 15 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == NULL)
    ASSERT_FORCE(buf.output_avail == -1)
    
    ChunkBuffer2_SubmitPacket(&buf, sizeof(struct ChunkBuffer2_block));
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[3])
    ASSERT_FORCE(buf.input_avail == 13 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.output_avail == 1 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_SubmitPacket(&buf, 8 * sizeof(struct ChunkBuffer2_block));
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[12])
    ASSERT_FORCE(buf.input_avail == 4 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.output_avail == 1 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_SubmitPacket(&buf, 4 * sizeof(struct ChunkBuffer2_block));
    
    ASSERT_FORCE(buf.input_dest == NULL)
    ASSERT_FORCE(buf.input_avail == -1)
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.output_avail == 1 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_ConsumePacket(&buf);
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.input_avail == 1 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[3])
    ASSERT_FORCE(buf.output_avail == 8 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_ConsumePacket(&buf);
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.input_avail == 10 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[12])
    ASSERT_FORCE(buf.output_avail == 4 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_SubmitPacket(&buf, 9 * sizeof(struct ChunkBuffer2_block));
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[11])
    ASSERT_FORCE(buf.input_avail == 0 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[12])
    ASSERT_FORCE(buf.output_avail == 4 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_ConsumePacket(&buf);
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[11])
    ASSERT_FORCE(buf.input_avail == 5 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.output_avail == 9 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_SubmitPacket(&buf, 1 * sizeof(struct ChunkBuffer2_block));
    
    ASSERT_FORCE(buf.input_dest == NULL)
    ASSERT_FORCE(buf.input_avail == -1)
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.output_avail == 9 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_ConsumePacket(&buf);
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.input_avail == 9 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == (uint8_t *)&blocks[11])
    ASSERT_FORCE(buf.output_avail == 1 * sizeof(struct ChunkBuffer2_block))
    
    ChunkBuffer2_ConsumePacket(&buf);
    
    ASSERT_FORCE(buf.input_dest == (uint8_t *)&blocks[1])
    ASSERT_FORCE(buf.input_avail == 15 * sizeof(struct ChunkBuffer2_block))
    ASSERT_FORCE(buf.output_dest == NULL)
    ASSERT_FORCE(buf.output_avail == -1)
    
    return 0;
}
