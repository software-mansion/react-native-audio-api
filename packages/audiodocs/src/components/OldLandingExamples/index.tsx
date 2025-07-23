import React from 'react';

import LandingExample from './LandingExample';
import StepSequencer from './StepSequencer';
import Piano from './Piano';

const LandingExamples: React.FC = () => (
  <>
    <LandingExample
      title="Lorem Ipsum"
      description="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
    >
      <StepSequencer />
    </LandingExample>
    <LandingExample
      title="Lorem Ipsum"
      description="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
      inverted
    >
      <Piano />
    </LandingExample>
    <LandingExample
      title="Lorem Ipsum"
      description="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
    />
    <LandingExample
      title="Lorem Ipsum"
      description="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
      inverted
    />
  </>
);

export default LandingExamples;
