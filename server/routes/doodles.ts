import express from 'express';
import * as z from 'zod';
import { randomUUID } from 'crypto';
import { exec } from 'child_process';

const router = express.Router();

const PostRequest = z.strictObject({
  script: z.string(),
  memory: z.int().positive(),
  cpuTime: z.int().positive(),
});

router.post('/', (req, res) => {
  const result = PostRequest.safeParse(req.body)
  if (!result.success) {
    res.status(400);
    res.json({
      message: 'Poorly formed request',
      errors: z.flattenError(result.error),
    });
    return;
  }

  const renderName = randomUUID();

  const doodle = exec(`./build/doodle >./build/renders/${renderName}`, (error) => {
    if (error) {
      res.status(400);
      res.json({ message: 'Failed to create image' });
    } else {
      res.status(201);
      res.json({
        message: 'Doodle created',
        name: renderName,
      });
    }
  });
  doodle.stdin?.write(result.data.script);
  doodle.stdin?.end();
})

export default router;
