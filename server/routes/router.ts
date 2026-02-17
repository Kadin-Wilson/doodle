import express from 'express';
import doodleRoutes from './doodles';

const router = express.Router();

router.use('/api/doodle', express.json(), doodleRoutes);

export default router;
