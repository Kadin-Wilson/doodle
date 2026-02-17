import express from 'express';
import router from './routes/router'

const app = express();

app.use('/', router);

app.listen(8000, () => console.log(`running on port 8000`));
