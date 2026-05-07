import React from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import CodeBlock from '@theme/CodeBlock';

const HELLO_SAMPLE = `import * from "lib/collections/ArrayList.mt";

class Person {
    private string name;
    private int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function toString(): string {
        return $"{this.name} (age {this.age})";
    }
}

function main(): void {
    ArrayList<Person> people = new ArrayList<Person>();
    people.add(new Person("Alice", 30));
    people.add(new Person("Bob", 25));
    for (Person p : people) {
        print(p.toString());
    }
}

main();
`;

type Feature = {
  title: string;
  description: string;
};

const FEATURES: Feature[] = [
  {
    title: 'Statically Typed OOP',
    description:
      'Classes, interfaces, abstract / final / value classes, generics, and pattern matching — with a strict type checker and clear diagnostics.',
  },
  {
    title: 'Bytecode VM + JIT',
    description:
      'AOT compile to .mtc bytecode and run on a stack-based VM. AsmJit-backed x86-64 JIT promotes hot paths automatically.',
  },
  {
    title: 'Async / Await',
    description:
      'First-class Promise<T> with cooperative scheduling on the VM event loop. async functions, await expressions, and structured concurrency.',
  },
  {
    title: 'Reflection & Annotations',
    description:
      'First-class Class, Method, Field, and Annotation reflection. Built-in annotations like @Script, @Throw, @Override plus user-defined annotations with @Retention / @Target.',
  },
  {
    title: 'Standalone Tooling',
    description:
      'mtpm package manager, .mtproj / .mtworkspace project system, standalone language server, and a full-featured VS Code extension.',
  },
  {
    title: 'Native Executables',
    description:
      'Build standalone executables that embed bytecode in a launcher binary — distribute apps without requiring users to install mType.',
  },
];

function HomepageHeader() {
  const { siteConfig } = useDocusaurusContext();
  return (
    <header className={clsx('hero')}>
      <div className="container">
        <h1 className="hero__title">{siteConfig.title}</h1>
        <p className="hero__subtitle">{siteConfig.tagline}</p>
        <div className="heroButtons">
          <Link
            className="button button--secondary button--lg"
            to="/docs/getting-started/install"
          >
            Get Started →
          </Link>
          <Link
            className="button button--outline button--secondary button--lg"
            to="/docs/language/classes"
          >
            Language Reference
          </Link>
          <Link
            className="button button--outline button--secondary button--lg"
            href="https://github.com/matan45/mType"
          >
            GitHub
          </Link>
        </div>
      </div>
    </header>
  );
}

function HomepageFeatures() {
  return (
    <section className="container">
      <div className="features">
        {FEATURES.map((f) => (
          <div className="feature" key={f.title}>
            <h3>{f.title}</h3>
            <p>{f.description}</p>
          </div>
        ))}
      </div>
    </section>
  );
}

function HomepageSample() {
  return (
    <section className="codePreview">
      <div className="container">
        <h2>A Quick Taste</h2>
        <CodeBlock language="mtype" title="hello.mt" showLineNumbers>
          {HELLO_SAMPLE}
        </CodeBlock>
        <p style={{ textAlign: 'center', marginTop: '1rem' }}>
          <Link to="/docs/getting-started/quick-start">Read the full Quick Start →</Link>
        </p>
      </div>
    </section>
  );
}

export default function Home(): React.ReactElement {
  const { siteConfig } = useDocusaurusContext();
  return (
    <Layout
      title={`${siteConfig.title} — ${siteConfig.tagline}`}
      description="mType is a statically typed OOP language with a bytecode VM, x86-64 JIT, async/await, generics, reflection, and a full toolchain (project system, package manager, language server, VS Code extension)."
    >
      <HomepageHeader />
      <main>
        <HomepageFeatures />
        <HomepageSample />
      </main>
    </Layout>
  );
}
